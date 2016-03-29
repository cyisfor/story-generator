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

struct Context(NodeType) {
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
	if(!in_paragraph) {
	  appendText("\n");
	  next(doc.createElement("p"));
	  appendText("\n");
	  in_paragraph = true;
	}
  }
  void maybe_end(string where) {
	if(in_paragraph) {
	  in_paragraph = false; // defer to next maybe_start
	}
  }
  void appendText(HTMLString text) {
	scope(failure) {
	  print("failed",e.lastChild.html);
	}
	if(this.e.isElementNode) {
	  print("uhm",text);
	  this.e.html(this.e.html ~ text);
	  print(e.html);
	} else {
	  this.e.text(this.e.text ~ text);
	}
  }
}

bool process_text(NodeType)(ref Context!NodeType ctx, HTMLString text) {
  import std.string: strip;
  import std.algorithm.iteration: splitter, map, filter;
  if(text.length == 0) return false;
  if(text[0] == '\n') {
	ctx.maybe_end("start");
  } else {
	ctx.ended_newline = text[$-1] == '\n';
  }
  auto lines = text.strip()
	.splitter('\n')
	.map!"a.strip()"
	.filter!"a.length > 0";
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
		print("cache!");
		cache[put] = r.front;
		r.popFront();
		++put;
		if(put == n) {
		  put = 0;
		  full = true;
		  break;
		}
	  }
	  print("mteee",r.empty);
	}
	bool empty() {
	  print("mt?",!full && put == get);
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
		print("pop?",r.empty);
		if(!r.empty) print(r.front);
	  }
	  print("put/get after pop",put,get,n,!full,put==get);
	}
  }
  CacheForward ret = {
	r = r
  };
  ret.initialize();
  return ret;
}
	  

void process_root(NodeType)(Document* dest, NodeType root, NodeType head) {
  import std.algorithm.searching: any;
  import std.array: array;  
  import print: print;
  auto ctx = Context!NodeType(dest,root);
  bool in_paragraph = false;

  foreach(e;array(root.children)) {
	if(e.isTextNode) {
	  if(process_text(ctx,e.text)) {
		e.detach();
	  }
	} else if(e.isElementNode) {
	  bool head_element =
		any!((a) => a == e.tag)
		(["title","meta","link","style","script"]);
	  if(head_element) {
		print("head",e.tag);
		e.detach();
		if(e.tag == "title") {
		  // no two titles!
		  foreach(tit; head.children) {
			if(tit.tag == "title")
			  tit.detach();
		  }
		}
		head.appendChild(e);
	  }
	  bool block_element =
		any!((a) => a == e.tag)
		(["ul","ol","p","div","table","blockquote"]);
	  if(block_element) {
		ctx.maybe_end("block");
		if(e.attr("hish")) {
		  e.removeAttr("hish");
		  process_root(dest,e, head);
		}
	  } else {
		/* start a paragraph if this element is a wimp
		   but only if the last text node ended on a newline.
		   otherwise the last text node and this should be in the same
		   paragraph */
		if(ctx.ended_newline) {
		  import std.format: format;
		  auto buf = format("wimp tag {{%s}}",e.tag);
		  ctx.maybe_end(buf);
		  ctx.maybe_start(buf);
		  ctx.ended_newline = false;
		}
	  }
	  ctx.next(e);
	} else {
	  ctx.next(e);
	}
  }
}

import std.typecons: NullableRef;

auto ref parse(string ident = "content",bool replace=false)
	(HTMLString source,
	 Document* templit = null) {
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
	}
  } else {
	// have to replace <content>
	auto derrp = derp.front;
	content = dest.createElement("div");
	foreach(e;derrp.children) {
	  content.appendChild(detach(e));
	}
	content.insertAfter(detach(derrp));
  }

  content.html(source);
  print("okay, process");
  process_root(dest,content, head);
  if(replace) {
	while(content.firstChild) {
	  auto c = content.firstChild;
	  detach(c).insertBefore(content);
	}
	content.detach();
  }
  assert(dest.root.document_ == dest,to!string(dest));
  return move(dest);
}

void make(string src, string dest, Document* templit = null) {
  import std.file: readText,write,rename;
  string source = readText(src);
  print("make",dest);
  (dest~".temp").write(parse(readText(src),templit).root.html);
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
