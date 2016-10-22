import print: print;
import std.conv: to;
import std.algorithm.mutation: move;
import htmlderp: detach;
import html: Document, HTMLString, createDocument;

Document* default_template;
static this() {
  // one to be there at compile time j/i/c
  default_template = createDocument(import("template/default.html"));
}

import fuck_selectors: by_name, by_class, by_id;

alias NodeType = typeof(default_template.root);

struct Context {
  bool ended_newline;
  bool in_paragraph;
  bool started;
  NodeType e;
  Document* doc;
  this(Document* doc, NodeType root) {
		this.doc = doc;
		this.e = root;
  }
  void next(NodeType e) {
		if(!started) {
			this.e.appendChild(detach(e));
			this.e = e;
			started = true;
		} else {
			if(in_paragraph) {
				this.e.appendChild(detach(e));
			} else {
				e.insertAfter(this.e);
				this.e = e;
			}
		}
  }
  void maybe_start(string where) {
		//print("start",where);
		if(!in_paragraph) {
			appendText("\n");
			next(doc.createElement("p"));
			appendText("\n");
			in_paragraph = true;
		}
  }
  void maybe_end(string where) {
		//print("end",where);
		if(in_paragraph) {
			in_paragraph = false; // defer to next maybe_start
		}
  }
  void appendText(HTMLString text) {
		scope(failure) {
			if(e.lastChild) {
				print("failed",e.lastChild.html);
			} else {
				print("uhh no last child");
			}
		}
		if(this.e.isElementNode) {
			if(this.e.lastChild && this.e.lastChild.isTextNode) {
				this.e.lastChild.text(this.e.lastChild.text ~ text);
			} else {
				this.e.appendText(text);
			}
		} else {
			this.e.text(this.e.text ~ text);
		}
   }
}

bool process_text(ref Context ctx, HTMLString text) {
  import std.string: strip;
  import std.algorithm.iteration: splitter, map, filter;
  import std.ascii: isWhite;
  if(text.length == 0) return false;
  bool start_space, end_space;
  size_t i;
  // mehhhhhh ignore tabs and spaces, but keep newlines?
  for(i=0;i<text.length;++i) {
    if(text[i] == '\n') {
      ctx.maybe_end("start");
      start_space = true;
      break;
    }
    if(!isWhite(text[i])) break;
    start_space = true;
  }
  for(i=0;i<text.length;++i) {
    long j = text.length - i - 1;
    if(!isWhite(text[j])) break;
    if(text[j] == '\n') {
      ctx.ended_newline = true;
      break;
    }
  }

  end_space = ctx.ended_newline || isWhite(text[$-1]);

  auto lines = text.strip()
		.splitter('\n')
		.map!"a.strip()"
		.filter!"a.length > 0";
  if(start_space) ctx.appendText(" ");
  if(lines.empty) return false;
  ctx.maybe_start("beginning");
  ctx.appendText(lines.front);
  lines.popFront();
  foreach(line; lines) {
		// end before start, to leave the last one dangling out there.
		ctx.maybe_end("middle");
		ctx.maybe_start("middle");
		ctx.appendText(line);
  }
  if(end_space) {
		ctx.appendText(" ");
  }
  return true;
}

auto cacheForward(int n = 2, Range)(Range r) {
  import std.range: ElementType;
  struct CacheForward {
		@disable this(this);
		Range r;
		ElementType!Range[n] cache;
		int put = 0;
		int get = 0;
		bool full = false;
		void initialize() {
			while(!r.empty) {
				cache[put] = r.front;
				r.popFront();
				++put;
				if(put == n) {
					put = 0;
					full = true;
					break;
				}
			}
		}
		bool empty() {
			return !full && put == get;
		}
		typeof(r.front()) front() {
			return cache[get];
		}
		void popFront() {
			assert(full || put != get);
			get = (get + 1) % n;
			if(r.empty) {
				full = false;
			} else {
				// never not full since we put for every get
				// until the underlying range is empty
				cache[put] = r.front;
				r.popFront();
				put = (put + 1) % n;
			}
		}
  }
  CacheForward ret = {
		r = r
  };
  ret.initialize();
  return ret;
}


NodeType process_chat(Document* dest, ref NodeType e) {
	import std.string: split;
	import std.algorithm.searching: findSplit;
	auto dl = dest.createElement("table");
	dl.insertBefore(e);
	e.detach();
	dl.attr("class","chat");
	foreach(line;e.html.split("\n")) {
		auto colon = line.findSplit(":");
		if(!colon) {
			continue;
		}
		auto tr = dest.createElement("tr");
		dl.appendChild(tr);
		auto append(string what, const char[] derp) {
			auto term = dest.createElement(what);
			term.appendChild(dest.createTextNode(derp));
			tr.appendChild(term);
			return term;
		}
		append("td",colon[0]);
		append("td",colon[1]);
		append("td",colon[2]);
	}
	return dl;
}

void process_root(Document* dest,
									NodeType root,
									ref NodeType head,
									ref string title) {
  import std.algorithm.searching: any;
  import std.array: array;
  auto ctx = Context(dest,root);
  bool in_paragraph = false;

  foreach(e;array(root.children)) {
		if(e.isTextNode) {
			import std.algorithm.searching: find;
			if(process_text(ctx,e.text)) {
				e.detach();
			}
			/*if(ctx.ended_newline)
				print("ENDED NEWLINE",e.text);	  */
		} else if(e.isElementNode) {
			switch(e.tag) {
			case "chat":
				ctx.next(process_chat(dest,e));
				break;
			case "title":
				auto maybetitle = e.html;
				if(maybetitle.length == 0) {
					// bork
					e.html = title;
				} else {
					title = to!string(maybetitle);
				}
				foreach(tit; head.children) {
					if(tit.tag == "title") {
						if(title.length > 0) {
							// no two titles!
							tit.destroy();
						} else {
							title = to!string(tit.html);
						}
					}
				}

				// get <intitle/>
				foreach(tit;dest.querySelectorAll("intitle")) {
					if(title !is null && title.length > 0) {
						dest.createTextNode(title).insertAfter(tit);
					}
					tit.destroy();
				}
				goto case;				
			case "meta":
			case "link":
			case "style":
			case "script":
				e.detach();
				head.appendChild(e);
        // NOT ctx.next(e);
				break;
			case "ul":
			case "ol":
			case "p":
			case "div":
			case "table":
			case "blockquote":
				ctx.ended_newline = false;
				//print("block element ",e.tag);
				ctx.maybe_end("block");
				if(e.attr("hish")) {
					e.removeAttr("hish");
					process_root(dest, e, head, title);
					//now the processed hish is inside e, and can be added just as if not hish
				}
				ctx.next(e);
				break;
			default:
				ctx.next(e);
				break;
			}
		} else {
			ctx.next(e);
		}
  }
}

import std.typecons: NullableRef;

auto ref parse(string ident = "content",
               bool replace=false)
	(HTMLString source,
	 Document* templit = null,
	 string title = null) {
  import std.algorithm.searching: until;
  import std.range: chain, InputRange;

  if(templit == null) {
		templit = default_template;
  }

  auto dest = templit.clone();
  assert(dest.root.document_ == dest,to!string(dest));

  auto head = dest.root.by_name!"head".front;

  // find where we're going to dump this htmlish
  auto derp = dest.root.by_name!(ident);
  typeof(derp.front) content;
  if(derp.empty) {
		auto dsux = chain(dest.root.by_id!(ident),
											dest.root.by_name!"body");

		if(dsux.empty) {
			import print: print;
			print("no content found!");
			content = dest.createElement("body");
			dest.root.appendChild(content);
		} else {
			content = dsux.front;
			print("content derp",content.outerHTML);

		}
  } else {
		// have to replace <content>
		auto derrp = derp.front;
    assert(derrp);
		content = dest.createElement("div");
		foreach(e;derrp.children) {
			content.appendChild(detach(e));
		}
		content.insertAfter(derrp);
    derrp.destroy();
  }
	auto temp = dest.createElement("div");
	temp.html(source);
	process_root(dest, temp, head, title);
	import std.array: array;
	foreach(child;temp.children.array) {
		child.detach();
		content.appendChild(child);
	}

	destroy(temp);
  //print("processing htmlish for",title);
  process_root(dest,content, head, title);
  auto titles = dest.querySelectorAll("title");
  if(titles.empty) {
    auto tite = dest.createElement("title");
    tite.html = title;
    head.appendChild(tite);
  } else {
    auto tite = titles.front;
    tite.html = title;
  }
  foreach(intit; dest.querySelectorAll("intitle")) {
    auto texttit = dest.createTextNode(title);
    texttit.insertBefore(intit);
    intit.detach();
  }
  if(replace) {
		while(content.firstChild) {
			auto c = content.firstChild;
			detach(c).insertBefore(content);
		}
		content.detach();
  }
	auto date = dest.createElement("div");
	date.attr("class","ddate");
	static import ddate;
	date.appendChild(dest.createTextNode(to!string(ddate.current())));
	dest.root.by_name!"body"
		.front.appendChild(date);
  assert(dest.root.document_ == dest,to!string(dest));
  return move(dest);
}

int derp = 0;

void make(string src, string dest, Document* templit = null) {
  import std.file: readText,write,rename;
  import htmlderp: querySelector;
  auto root = parse(readText(src),templit).root;

  // append to the file in place
  import std.stdio;
  struct Derpender {
		File ou;
		void put(T)(T s) {
			ou.write(to!string(s));
			ou.flush();
		}
  }
  File destf = File(dest~".temp","wt");
  scope(exit) destf.close();
  auto app = Derpender(destf);
  root.innerHTML(app);
  rename(dest~".temp",dest);
}

auto make(Document* templit) {
	void derp(string src, string dest) {
		make(src,dest,templit);
	}
	return &derp;
}

unittest {
  import std.stdio: writeln,stdout;
  static import backtrace;
  static backtrace.PrintOptions opts = {
	colored:  true,
	detailedForN: 5,
  };
  backtrace.install(stdout,opts,5);
  auto p = parse(`hi
there
this
is <i>a</i> test`);
  writeln(p.root.html);
}

unittest {
  import std.stdio;
  auto p = parse(`herp
Italics should <i>not</i> be shoved to the back.
derp`);
  writeln(p.root.html);
}

unittest {
  import std.stdio;
  auto p = parse(`after <div hish="1"> a <u>tag</u>
With newlines, it should be a new paragraph.</div>

With only space at the <u>end</u>
It should still be a new paragraph.`);
  writeln(p.root.html);
}
