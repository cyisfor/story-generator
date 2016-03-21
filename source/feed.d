import db: Story, Chapter;

import html: createDocument,Document;

import std.datetime: SysTime;
import std.conv: to;
import std.file: write;

const string preamble = `<?xml version="1.0" encoding="UTF-8"?>`;

string encode_date(SysTime s) {
  // %Y-%m-%dT%H:%M%:S
  return s.toISOExtString();

}

class Feed {
  struct Params {
	string title;
	string url;
	string feed_url;
	string id;
	string content;
	string author;
	SysTime updated;
  }
  Params p;

  Document doc;
  typeof(doc.root) feed;

  this(Params params) {
	this.p = params;
	doc = createDocument(`<feed xmlns="http://www.w3.org/2005/Atom"/>`);
	feed = doc.root;

	doc.createElement("id",feed).appendText(p.id);
	doc.createElement("title",feed).appendText(p.title);
	auto link = doc.createElement("link",feed);
	link.attr("href", p.url);
	link.attr("rel", "alternate");
	link = doc.createElement("link",feed);
	link.attr("href", p.feed_url);
	link.attr("rel", "self");
	doc.createElement("updated",feed).appendText(encode_date(p.updated));
  }

  auto start_out(string url) {
	auto entry = doc.createElement("entry",feed);
	auto link = doc.createElement("link",entry);
	link.attr("href", url);
	link.attr("rel","alternate");
	link.attr("type", "text/xml+xhtml");
	doc.createElement("author",entry).appendText("Skybrook");
	return entry;
  }
  void put(Story story) {
	auto entry = start_out(story.url);
	if(story.description != null) {
	  auto content = doc.createElement("content",entry);
	  content.html = story.description;
	  content.attr("type","text/html");
	}
	doc.createElement("title",entry).appendText(story.title);
  }

  void put(Chapter chapter) {
	auto entry = start_out(chapter.story.url ~ "/chapter" ~
						   to!string(chapter.which));
	if(chapter.first_words != null) {
	  auto content = doc.createElement("content",entry);
	  content.html = chapter.first_words;
	  content.attr("type","text/html");
	}
	doc.createElement("title",entry).appendText(chapter.title);
  }

  void save(string dest) {
	write(dest,
		  preamble ~
		  doc.root.outerHTML);
  }
}
