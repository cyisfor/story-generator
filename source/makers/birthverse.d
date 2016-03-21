module makers.birthverse;
import htmlderp: createElement;
static import makers;

import arsd.dom: Document;

import std.file: readText,write;
import std.stdio: File;
import std.conv: to;
import std.string: strip;

class Maker : makers.Maker {
  override
  void make(string src, string dest) {
	auto doc = new Document(readText("template/birthverse.xhtml"));
	auto bod = doc.querySelector("div.greentext");
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
	doc.querySelector("title").appendChild(tnode);
	foreach(e; doc.querySelectorAll("intitle")) {
	  // can't reuse, because it'll relink parent / has move semantics
	  tnode = doc.createTextNode(title);
	  tnode.insertAfter(e);
	  e.detach();
	}
	write(dest,doc.root.outerHTML);
  }
}

static this() {
  import makers: makers;
  makers["birthverse"] = Maker();
}
