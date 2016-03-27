module makers.littlepip;
import print: print;

import htmlderp: querySelector;
import html: Document;

import core.exception: AssertError;

void add_style(Document* doc) {
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
	print(doc.root.outerHTML);
	throw(e);
  }
}

static this() {
  static import makers;
  import std.functional: toDelegate;
  makers.contents["littlepip"] = toDelegate(&add_style);
  makers.chapter["littlepip"] = toDelegate(&add_style);
}
