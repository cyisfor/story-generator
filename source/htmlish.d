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
	this.e = root;
  }
  void next(auto e) {
	if(in_paragraph) {
	  this.e.appendChild(e);
	} else {
	  this.e.insertAfter(e);
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
  if(!text) return ;
  if(text[0] == '\n') {
	ctx.maybe_end("start");
  } else {
	ctx.ended_newline = text[$] == '\n';
  }
  foreach(line; text.strip().split('\n')) {
	line = line.strip();
	if(line.empty) continue;
	ctx.maybe_start("beginning");
	e.text ~= line;
	ctx.maybe_end("beginning");
  }
}

auto process_root(NodeType)(Document!NodeType dest) {
  return (auto root) {
	if(!root.attr("hish")) return false;
	root.removeAttr("hish");
	Context ctx = Context(root);
	bool in_paragraph = false;
	foreach(e; root.children) {
	  auto next = e.next;
	  if(e.isTextNode) {
		process_text(e.text);
	  } else if(e.isElementNode) {
		bool block_element =
		  any!((a) => a == e.name)
		  (["ul","ol","p","div","table","blockquote"]);
		if(block_element) {
		  ctx.maybe_end("block");
		  if(process_root(e,ctx)) {
			e = next;
			continue;
		  }
		} else {
		  /* start a paragraph if this element is a wimp
			 but only if the last text node ended on a newline.
			 otherwise the last text node and this should be in the same
			 paragraph */
		  if(ctx.ended_newline) {
			auto buf = format("wimp tag {{%s}}",e.name);
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
	return true;
  }
}
  
auto parse(HTMLString source, ref Document templit = null) {
  import std.algorithm.searching: until;
  import std.range: chain;
  import htmlderp: createDocument;
  
  if(templit == null) {
	templit = default_template;
  }

  auto dest = templit.clone();
  
  // find where we're going to dump this htmlish
  auto content = until!"a !is null"
	(chain(dest.root.by_name!"content",
		   dest.root.by_name!"div".by_id!"content",
		   dest.root.by_name!"body")).front;
  if(content is null) {
	content = dest.createElement("body");
	dest.root.appendChild(content);
  }
  
  auto src = createDocument(source);
  assert(src.root);

  content.html(source);
  process_root(dest)(content);
  return dest;
}

void make(string src, string dest) {
  import std.file: readText,write;
  string source = readText(src);
  (dest~".temp").write(parse(readText(src)).root.html);
  rename(dest~".temp",dest);
}
