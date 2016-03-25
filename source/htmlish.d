import print: print;
import htmlderp: createDocument, querySelector;
import html: Document, HTMLString;

import std.stdio: File;
import std.file: rename;

Document default_template;
static this() {
  // one to be there at compile time j/i/c
  default_template = createDocument(import("template/default.html"));
}

import fuck_selectors: for_all, is_element, by_name, by_class, by_id;

struct Context(NodeType) {
  bool ended_newline;
  bool in_paragraph;
  NodeType e;
  Document doc;
  this(Document doc, NodeType root) {
	this.doc = doc;
	this.e = root.firstChild;
  }
  void next(NodeType e) {
	if(in_paragraph) {
	  this.e.appendChild(e);
	} else {
	  this.e = e;
	}
  }
  void maybe_start(string where) {
	if(!in_paragraph) {
	  e.appendText("\n");
	  next(doc.createElement("p"));
	  e.appendText("\n");
	  in_paragraph = true;
	}
  }
  void maybe_end(string where) {
	if(in_paragraph)
	  in_paragraph = false; // defer to next maybe_start
  }
}

void process_text(NodeType)(Context!NodeType ctx, HTMLString text) {
  import std.string: strip;
  import std.algorithm.iteration: splitter, map;
  if(text.length == 0) return ;
  if(text[0] == '\n') {
	ctx.maybe_end("start");
  } else {
	ctx.ended_newline = text[$-1] == '\n';
  }
  auto lines = text.strip()
	.splitter('\n')
	.map!"a.strip()"
	.filter!"a.length > 0";
  if(lines.empty) return;
  ctx.maybe_start("beginning");
  ctx.e.appendText(lines.front);
  lines.popFront();
  foreach(line; lines) {
	line = line.strip();
	if(line.length==0) continue;
	// end before start, to leave the last one dangling out there.
	ctx.maybe_end("middle");
	ctx.maybe_start("middle");
	ctx.e.appendText(line);
  }
  
}

void process_root(NodeType)(Document dest, NodeType root) {
  import std.algorithm.searching: any;
  import print: print;
  print("uhm",root);
  auto ctx = Context!NodeType(dest,root);
  bool in_paragraph = false;
  foreach(e; root.children) {
	auto next = e.nextSibling;
	print("hhhhmmm",e);
	if(e.isTextNode) {
	  process_text(ctx,e.text);
	} else if(e.isElementNode) {
	  bool block_element =
		any!((a) => a == e.tag)
		(["ul","ol","p","div","table","blockquote"]);
	  if(block_element) {
		ctx.maybe_end("block");
		if(e.attr("hish")) {
		  e.removeAttr("hish");
		  process_root(dest,e);
		  e = next;
		  continue;
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
	} else {
	  ctx.next(e);
	}
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
	  dest.root.appendChild(content);
	} else {
	  content = dsux.front;
	}
  } else {
	// have to replace <content>
	content = dest.createElement("div");
	foreach(e;derp.front.children) {
	  e.detach();
	  content.appendChild(e);
	}
	content.insertAfter(derp.front);
	derp.front.detach();
  }

  auto src = createDocument(source);
  assert(src.root);

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
  import std.stdio: writeln;
  writeln(parse(`hi
there
this
is <i>a</i> test`).root.html);
  }
