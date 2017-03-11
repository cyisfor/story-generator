import print : print;
import htmlderp : querySelector;
import feed : Feed;
import db : Story, Chapter;

static import db;
import makers;

import html: Document, createDocument;

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

Document* contents;
static this() {
	contents = createDocument(import("template/contents.xhtml"));
}

immutable string[] title_images =
	["title.jpg",
	 "title.png",
	 "title.gif",
	 "cover.jpg",
	 "cover.png"];

SysTime reindex(string outdir, Story story) {
		static import htmlish;
		import std.algorithm.mutation: move;
		import std.file: exists;
		if(story.description == null) {
			story.edit();
		}
				print("doing index",story.title);
		auto doc = htmlish.parse!"description"(story.description,
																							 contents,
																							 story.title);
		bool foundit = false;
		auto title_image = querySelector(doc,"img#title");
		foreach(img; title_images) {
			auto path = buildPath(story.location,img);
			if(exists(path)) {
				title_image.attr("src","../../" ~ path);
				print("found title image",path,title_image);
				foundit = true;
				break;
			}
		}
		if(!foundit) {
			title_image.parent.removeChild(title_image);
		}
	try {
		auto toc = querySelector(doc, "#toc");

		SysTime maxTime = SysTime(0);
		if (story.chapters == 0) {
			print("updoot");
			story.update();
		}
		print(story.id,story.title,"has",story.chapters,"chapters");
		int last;
		if(story.finished)
			last = story.chapters;
		else
			last = story.chapters - 1;
		for (int which = 0; which < last; ++which) {
			auto chapter = story.get!true(which);
			maxTime = max(maxTime, chapter.modified);
			auto link = doc.createElement("a",
					doc.createElement("li", toc));
			link.attr("href", chapter_name(which)~".html");
			if(chapter.title.length==0) {
				link.appendText("(untitled)");
			} else {
				link.appendText(chapter.title);
			}
		}
		if (auto box = story.location in makers.contents) {
			(*box)(doc);
		}
		if (story.modified < maxTime) {
			story.update();
		}
		import std.file: write;
		auto dest = buildPath(outdir,story.location,"contents.html");				
				dest.write(doc.root.html);
				dest.setTimes(maxTime,maxTime);
				buildPath(outdir,story.location).setTimes(maxTime,maxTime);
		return maxTime;
	}
	catch (AssertError e) {
		print(doc.root.outerHTML);
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
	import std.algorithm.iteration: map;

	auto sorted = make!
		(RedBlackTree!(Story,"a.modified < b.modified", true))
		(stories.byValue);
	foreach (Story story; sorted) {
		print("story", story.location, story.modified);
		assert(story.location);
		SysTime modified = reindex(outdir, story);
		setTimes(story.location, modified, modified);
		maxTime = max(modified, maxTime);
	}

	print(stories.length, "stories at", maxTime);
	//dfmt off
	Feed.Params p = {
		title: "Recently Updated",
		url: "http://[fcd9:e703:498e:5d07:e5fc:d525:80a6:a51c]/stories/",
		name: "updates.atom",
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
