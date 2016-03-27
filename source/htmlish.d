import print: print;
import std.conv: to;
import std.algorithm.mutation: move;
import htmlderp: createDocument, detach;
import html: Document, HTMLString;

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
		e.insertBefore(this.e);
		this.e = e;
	  }
	}
  }
  void maybe_start(string where) {
	if(!in_paragraph) {
	  appendText("\n");
	  next(detach(doc.createElement("p")));
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
	  this.e.appendChild(detach(doc.createTextNode(text)));
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

void process_root(NodeType)(Document* dest, NodeType root) {
  import std.algorithm.searching: any;
  import std.algorithm.iteration: cache;
  import print: print;
  auto ctx = Context!NodeType(dest,root);
  bool in_paragraph = false;

  foreach(e;cache(root.children)) {
	if(e.isTextNode) {
	  if(process_text(ctx,e.text)) {
		e.detach();
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
  }
}

import std.typecons: NullableRef;

auto ref parse(string ident = "content")
	(HTMLString source, 
	 Document* templit = null) {
  import std.algorithm.searching: until;
  import std.range: chain, InputRange;
  import htmlderp: createDocument;
  
  if(templit == null) {
	templit = default_template;
  }

  auto dest = templit.clone();
  assert(dest.root.document_ == dest,to!string(dest));
  
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
	  dest.root.appendChild(detach(content));
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
	content.insertBefore(derrp);
	derrp.detach();
  }

  content.html(source);
  process_root(dest,content);
  assert(dest.root.document_ == dest,to!string(dest));
  return move(dest);
}

void make(string src, string dest, Document* templit = null) {
  import std.file: readText,write,rename;
  string source = readText(src);
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
