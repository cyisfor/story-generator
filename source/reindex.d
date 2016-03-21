import htmlderp: querySelector;
import feed: Feed;
import db: Story, Chapter;
static import db;
import makers: makers;

import html: createDocument;

import std.file: setTimes;
import std.path: buildPath;

version(GNU) {
  import std.algorithm: copy, max;
} else {
  import std.algorithm.mutation: copy;
  import std.algorithm.comparison: max;
 }
import std.datetime: SysTime;
import std.conv: to;
import std.stdio;
import core.exception: AssertError;
string chapter_name(int which) {
  if(which == 0) return "index";
  return "chapter" ~ to!string(which+1);
}

SysTime reindex(Story story) {
  auto contents = createDocument(import("template/contents.xhtml"));
  assert(story.description != null);
  auto desc = querySelector(contents,"#description");
  desc.html = story.description;
  try {
	auto toc = querySelector(contents,"#toc");

  SysTime maxTime = SysTime(0);
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
void reindex(string outdir, Story[string] stories) {
  SysTime maxTime = SysTime(0);
  if(stories.length == 0) {
	writeln("no stories to update?");
	return;
  }
  foreach(string location, Story story; stories) {
	writeln("story ",location);
	story.location = location;
	SysTime modified = reindex(story);
	setTimes(location, modified, modified);
	maxTime = max(modified,maxTime);
  }

  writeln(stories.length," at ",maxTime);
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

  recent.save(buildPath(outdir,"updates.atom"));
}
