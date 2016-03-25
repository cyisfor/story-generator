static import backtrace;
import print: print;
import htmlderp: createDocument, querySelector, detach;
import html: Document, HTMLString;

import std.string: strip;
import std.stdio: File;
import std.file: rename;

Document default_template;
static this() {
  // one to be there at compile time j/i/c
  default_template = createDocument(import("template/default.html"));
}

import fuck_selectors: by_name, by_class, by_id;

void sanity_check(T)(T ee) {
  assert(ee.lastChild.isNull ||
		 ee.lastChild.nextSibling.isNull,
		 ee.html);
  bool[typeof(ee)] cycles;
  import std.array: appender;
  auto seen = appender!(typeof(ee)[]);
  foreach(e; ee.descendants) {
	if(e in cycles) {
	  import std.conv: to;
	  print(seen.data);
	  throw new Exception(to!string("cycle detected..." ~ to!string(e) ~ "<- " ~ to!string(cycles.keys)));
	  void dump(int indent, typeof(e) e) {
		foreach(ee; e.children) {
		  import std.stdio;
		  print(indent,to!string(ee));
		  if(ee == e) {
			print("here!");
		  } else {
			dump(indent+1,ee);
		  }
		}
	  }
	  dump(0,e);
	}
	cycles[e] = true;
	seen.put(e);
  }

}
  

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
	sanity_check(this.e);
	sanity_check(e);
	if(!started) {
	  this.e.appendChild(detach(e));
	  sanity_check(this.e);
	  sanity_check(e);
	  this.e = e;
	  started = true;
	} else {
	  if(in_paragraph) {
		this.e.appendChild(detach(e));
		sanity_check(this.e);
	  } else {
		this.e.insertAfter(detach(e));
		print("oy",e.html);
		this.e = e;
	  }
	}
  }
  void maybe_start(string where) {
	if(!in_paragraph) {
	  print("start",where);
	  appendText("\n");
	  sanity_check(e);
	  next(detach(doc.createElement("p")));
	  sanity_check(e);
	  appendText("\n");
	  sanity_check(e);
	  in_paragraph = true;
	}
  }
  void maybe_end(string where) {
	if(in_paragraph) {
	  print("end",where);		
	  in_paragraph = false; // defer to next maybe_start
	}
  }
  void appendText(HTMLString text) {
	scope(failure) {
	  print("failed",e.lastChild.html);
	}
	if(this.e.isElementNode) {
	  this.e.appendChild(detach(doc.createTextNode(text)));
	  sanity_check(this.e);
	} else {
	  print("whoop",text);
	  this.e.text(this.e.text ~ text);
	  sanity_check(e);
	}
  }
}

bool process_text(NodeType)(ref Context!NodeType ctx, HTMLString text) {
  import std.string: strip;
  import std.algorithm.iteration: splitter, map, filter;
  if(text.length == 0) return false;
  if(text[0] == '\n') {
	ctx.maybe_end("start");
	sanity_check(ctx.e);
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
  sanity_check(ctx.e);
  print("begin",lines.front);
  lines.popFront();
  foreach(line; lines) {
	print("mid",line);
	// end before start, to leave the last one dangling out there.
	ctx.maybe_end("middle");
	ctx.maybe_start("middle");
	sanity_check(ctx.e);
	ctx.appendText(line);
	sanity_check(ctx.e);
  }
  sanity_check(ctx.e);			
  return true;  
}

void process_root(NodeType)(ref Document dest, NodeType root) {
  import std.algorithm.searching: any;
  import print: print;
  print("uhm",root);
  auto ctx = Context!NodeType(&dest,root);
  bool in_paragraph = false;
  auto e = root.firstChild;
  while(e.node_) {
	auto next = e.nextSibling;
	bool noprev = false;
	if(next.isNull) {
	  noprev = true;
	} else {
	  next = next.nextSibling;
	}
	print("hhhhmmm",e.outerHTML);
	if(e.isTextNode) {
	  sanity_check(ctx.e);
	  if(process_text(ctx,e.text)) {
		sanity_check(ctx.e);
		e.detach();
	  } else {
		sanity_check(ctx.e);
	  }
	} else if(e.isElementNode) {
	  bool block_element =
		any!((a) => a == e.tag)
		(["ul","ol","p","div","table","blockquote"]);
	  if(block_element) {
		ctx.maybe_end("block");
		if(e.attr("hish")) {
		  e.removeAttr("hish");
		  process_root(dest,e);
		}
	  } else {
		/* start a paragraph if this element is a wimp
		   but only if the last text node ended on a newline.
		   otherwise the last text node and this should be in the same
		   paragraph */
		sanity_check(ctx.e);
		if(ctx.ended_newline) {
		  import std.format: format;
		  auto buf = format("wimp tag {{%s}}",e.tag);
		  ctx.maybe_end(buf);
		  ctx.maybe_start(buf);
		  ctx.ended_newline = false;
		}
	  }
	  ctx.next(detach(e));
	} else {
	  ctx.next(detach(e));
	}
	if(!noprev)
	  next = next.previousSibling;
	e = next;
  }
}

import std.typecons: Nullable;

auto parse(HTMLString source, Nullable!Document templit = Nullable!Document()) {
  import std.algorithm.searching: until;
  import std.range: chain, InputRange;
  import htmlderp: createDocument;
  
  if(templit.isNull) {
	templit = default_template;
  }

  auto dest = templit.clone();
  
  // find where we're going to dump this htmlish
  auto derp = dest.root.by_name!"content";
  typeof(derp.front) content;
  if(derp.empty) {
	auto dsux = chain(dest.root.by_name!"div".by_id!"content",
					  dest.root.by_name!"body");
	
	if(dsux.empty) {
	  import print: print;
	  print("no content found!");
	  print(dest.root.html);
	  content = dest.createElement("body");
	  dest.root.appendChild(detach(content));
	} else {
	  content = dsux.front;
	}
  } else {
	// have to replace <content>
	derp.front.appendText("huhh?");
	auto derrp = derp.front;
	content = dest.createElement("div");
	foreach(e;derrp.children) {
	  print("boing?",e);
	  content.appendChild(detach(e));
	}
	derrp.insertAfter(detach(content));
	dest.root.appendChild(content);
	print("ugh",dest.root.outerHTML);
	assert(!content.nextSibling.isNull);
	derrp.detach();
	content.appendText("foo");
	print("ugh",dest.root.outerHTML);
  }

  content.html(source);
  process_root(dest,content);
  return dest;
}

void make(string src, string dest) {
  import std.file: readText,write;
  string source = readText(src);
  (dest~".temp").write(parse(readText(src)).root.html);
  rename(dest~".temp",dest);
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
  print("beep");
  writeln(p.root.html);
}
