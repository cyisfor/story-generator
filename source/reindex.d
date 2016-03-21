import htmlderp: createElement, querySelector;
import feed: Feed;
import db: Story, Chapter;
static import db;
import makers: makers;

import arsd.dom: Document;

import std.file: setTimes;
import std.algorithm.mutation: copy;
import std.algorithm.comparison: max;
import std.datetime: SysTime;
import std.conv: to;
import std.stdio;
import core.exception: AssertError;
string chapter_name(int which) {
  if(which == 0) return "index";
  return "chapter" ~ to!string(which+1);
}

SysTime reindex(Story story) {
  Document contents = new Document(import("template/contents.xhtml"));
  assert(story.description != null);
  auto desc = querySelector(contents,"#description");
  desc.html = story.description;
  try {
	auto toc = querySelector(contents,"#toc");

  SysTime maxTime;
  Chapter[] chapters;
  for(int which=0;which<story.chapters;++which) {
	auto chapter = story[which];
	maxTime = max(maxTime,chapter.modified);
	auto link = contents.createElement("a",contents.createElement("li",toc));
	link.attr("href",chapter_name(which));
  }
  if(story.location in makers) {
	makers[story.location].contents(contents);
  }
  return maxTime;
  } catch(AssertError e) {
	writeln(contents.root.outerHTML);
	throw e;
  }
}
void reindex(Story[string] stories) {
  SysTime maxTime;
  foreach(string location, Story story; stories) {
	story.location = location;
	SysTime modified = reindex(story);
	setTimes(location, modified, modified);
	maxTime = max(modified,maxTime);
  }
  if(maxTime == SysTime()) return;

  Feed.Params p = {
  title: "Recently Updated",
  url: "./",
  feed_url: "updates.atom",
  id: "updated-derp-etc",
  content: "Recently Updated Stories",
  author: "Skybrook",
  updated: maxTime
  };
  Feed recent = new Feed(p);


  copy(db.latest(),recent);

  recent.save("out/updates.atom");
}
