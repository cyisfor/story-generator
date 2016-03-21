module makers.birthverse;
import htmlderp: querySelector;
import html: createDocument;

import std.file: readText,write;
import std.stdio: File, writeln;
import std.conv: to;
import std.string: strip;

void make(string src, string dest) {
  auto doc = createDocument(readText("template/birthverse.xhtml"));
  auto bod = querySelector(doc, "div.greentext");
  assert(bod);

  auto inp = File(src, "r");
  scope(exit) { inp.close(); }
  typeof(doc.root) ul;
  string title = null;
  foreach(derp; inp.byLine) {
	string line = to!string(derp).strip();
	if( line.length == 0 ) continue;
	if( title == null ) {
	  title = line;
	  continue;
	}
	if( ul == null ) {
	  ul = doc.createElement("ul",bod);
	}
	auto li = doc.createElement("li",ul);
	li.html = line;
	if( to!string(li.text).strip() == "" ) {
	  // only comment?
	  li.detach();
	}
  }

  assert(title,"no title in " ~ src);
  auto tnode = 	doc.createTextNode(title);
  querySelector(doc, "title").appendChild(tnode);
  foreach(e; doc.querySelectorAll("intitle")) {
	// can't reuse, because it'll relink parent / has move semantics
	tnode = doc.createTextNode(title);
	tnode.insertAfter(e);
	e.detach();
  }
  write(dest,doc.root.outerHTML);
  writeln("okie wrote to ",dest);
}

static this() {
  static import makers;
  makers.make["birthverse"] = &make;
}
