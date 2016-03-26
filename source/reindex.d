import print : print;
import htmlderp : querySelector, createDocument;
import feed : Feed;
import db : Story, Chapter;

static import db;
import makers;

import std.file : setTimes;
import std.path : buildPath;

version (GNU) {
	import std.algorithm : copy, max;
}
else {
	import std.algorithm.mutation : copy;
	import std.algorithm.comparison : max;
}
import std.datetime : SysTime;
import std.conv : to;
import std.stdio;
import core.exception : AssertError;

string chapter_name(int which) {
	if (which == 0)
		return "index";
	return "chapter" ~ to!string(which + 1);
}

Document contents;
static this() {
	contents = createDocument(import("template/contents.xhtml"));
}

SysTime reindex(Story story) {
	try {
		auto toc = querySelector(contents, "#toc");

		SysTime maxTime = SysTime(0);
		if (story.chapters == 0) {
			print("updoot");
			story.update();
		}
		for (int which = 0; which < story.chapters; ++which) {
			auto chapter = story[which];
			maxTime = max(maxTime, chapter.modified);
			auto link = contents.createElement("a",
					contents.createElement("li", toc));
			link.attr("href", chapter_name(which));
		}
		if (auto box = story.location in makers.contents) {
			(*box)(contents);
		}
		if (story.modified < maxTime) {
			story.update();
		}
		return maxTime;
	}
	catch (AssertError e) {
		print(contents.root.outerHTML);
		throw e;
	}
}

void reindex(string outdir, Story[string] stories) {
	SysTime maxTime = SysTime(0);
	if (stories.length == 0) {
		print("no stories to update?");
		return;
	}
	import std.container.rbtree : RedBlackTree;
	import std.container.util : make;

	auto sorted = make!(RedBlackTree!(Story, "a.modified < b.modified", true))(
			stories.values);
	foreach (Story story; sorted) {
		print("story", story.location, story.modified);
		assert(story.location);
		SysTime modified = reindex(story);
		setTimes(story.location, modified, modified);
		maxTime = max(modified, maxTime);
	}

	print(stories.length, "chapters at", maxTime);
	//dfmt off
	Feed.Params p = {
		title: "Recently Updated",
		url: "./",
		feed_url: "updates.atom",
		id: "updated-derp-etc",
		content: "Recently Updated Stories",
		author: "Skybrook",
		updated: maxTime
	};
	//dfmt on
	Feed recent = new Feed(p);

	copy(db.latest(), recent);

	recent.save(buildPath(outdir, "updates.atom"));
}
