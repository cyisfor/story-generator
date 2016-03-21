module makers.littlepip;

import htmlderp: querySelector;
import html: Document;

import std.stdio: writeln;
import core.exception: AssertError;

void add_style(ref Document doc) const {
  try {
	auto link = doc.createElement("link");
	auto head = querySelector(doc, "head");
	assert(head != null);
	assert(head.node_ != null);
	link.attr("href","style.css"); // style for this story alone
	link.attr("rel","stylesheet");
	link.attr("type","text/css");
	head.appendChild(link);
  } catch(AssertError e) {
	writeln(doc.root.outerHTML);
	throw(e);
  }
}

void contents(Document doc) const {
  add_style(doc);
}

void chapter(Document doc) const {
  add_style(doc);
}


static this() {
  static import makers;
  makers.contents["littlepip"] = &add_style;
  makers.chapter["littlepip"] = &add_style;
}
