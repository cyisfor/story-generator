static this() {
	static import backtrace;
	import std.stdio: stdout;
	backtrace.PrintOptions options = {
	colored: true,
	detailedForN: 5,
	};
	backtrace.install(stdout,options);
}

static import db;

import std.array: appender, Appender;

import reindex: reindex, chapter_name;
import std.file: exists;
import std.path: dirName;

import html: createDocument;

import print: print;
import std.conv: to;
import std.datetime : SysTime;
import std.algorithm: move;
import std.functional: memoize;

db.Story[string] stories;

bool[string] need_reindex;

void delegate(string,string) default_make_chapter;

static import htmlish;
static this() {
	default_make_chapter = htmlish.make
		(createDocument(import("template/chapter.xhtml")));
}

void smackdir(string p) {
	import std.file: mkdir;
	try {
		mkdir(p);
	} catch (Exception e) {}
}

bool update_last = false;
bool only_until_current = false;

struct Update {
	@disable this(this);
	db.Story* story;
	SysTime modified;
	int which;
	string location;
	string markup;
	string dest;
	string name;

	void perform() {

		// don't do anything on the last chapter, if multiple chapters
		// and there's more than 1 chapter.

		if(story.finished) {
			print("story set as finished",story.title);
		} else if(update_last) {
			print("all stories set to include last chapter");
		} else if(story.chapters > 1 &&
							(!story.finished) &&
							(which >= story.chapters - 1)) {
			print("Not updating last chapter",which,story.chapters);
			// undo possible side chapter before
			auto sidekey = Upd8(location, which-1);
			if(sidekey in one_before) {
				print("not updating previous (side) chapter",which-1);
				// note, since side chapters are queued AFTER regular ones, if the
				// regular one is the last, then no_update will capture the side chapter
				no_update[sidekey] = true;
			}
			return;
		} else {
			auto key = Upd8(location, which);
			if(key in no_update) return;
		}
		print("updating ugh",which,story.chapters,which >= story.chapters-1);
		noseriously();
	}
	void noseriously() {
		need_reindex[location] = true;
		// unconditionally update
		auto transaction = db.transaction();
		scope(success) transaction.commit();
		import std.file: setTimes;
		import htmlderp: querySelector;
		import makers;
		static import nada = makers.birthverse;
		static import nada2 = makers.littlepip;
		import std.file: write, readText;
		import std.path: buildPath;

		// find a better place to put stuff so it doesn't scram the source directory
		string basedir = "tmp";
		smackdir(basedir);
		basedir = buildPath(basedir,"base");
		smackdir(basedir);
		basedir = buildPath(basedir,location);
		smackdir(basedir);
		auto herpaderp() {
			if(auto box = location in makers.make) {
				print("found maker at ",location);
				return *box;
			} else {
				htmlish.modified = modified.toUnixTime();
				return default_make_chapter;
			}
		}
		auto make = herpaderp();

		print("creating chapter",which,markup,story.id,name);

		auto base = buildPath(basedir,name ~ ".html");
		make(markup,base);
		auto doc = createDocument
			(readText(buildPath(basedir, name ~ ".html")));

		auto head = querySelector(doc,"head");
		auto links = doc.querySelector("#links");
		if(links is null) {
			links = doc.createElement("div", querySelector(doc,"body"));
			links.attr("class","links");
		}
		void dolink(string href, string rel, string title) {
			auto link = doc.createElement("link",head);
			link.attr("rel",rel);
			link.attr("href",href);

			link = doc.createElement("a",links);
			link.attr("href",href);
			link.appendText(title);
		}
		if( which > 0 ) {
			dolink(chapter_name(which-1)~".html", "prev", "Previous");
			links.appendText(" ");
		} else {
			links.appendText("Previous ");
		}
		/* only add a Next link if
			 * there is a next chapter
			 * the next chapter isn't the last chapter, and the story isn't finished
			   (since that one's always in progress)
			 * OR
			 * this chapter isn't the "current" chapter
			   (if we're only updating to the current chapter)
		*/
		int derp = update_last ? 1 : (story.finished ? 1 : 2);
		print("urgh",derp,which,story.chapters);
		bool doit() {
			if( which + 1 == story.chapters ) return false;
			if (only_until_current) {
				if(story.current_chapter > 0 &&
					 which == story.current_chapter) return false;
			} else {
				if (which + derp == story.chapters) return false;
			}
			return true;
		}
		if(doit()) {
			dolink(chapter_name(which+1)~".html", "next", "Next");
			links.appendText(" ");
		} else {
			links.appendText("Next ");
		}
		dolink("contents.html","first","Contents");

		if(auto box = location in makers.chapter) {
			(*box)(doc);
		}

		string title = null;
		{
			// does this chapter have a title?
			auto res = doc.querySelectorAll("title");
			if(!res.empty) {
				title = to!string(res.front.html);
			}
		}

		// update chapter in db
		story.get(which)
			.update(modified,
							which,
							title);

		print("writing",location, which,dest);
		write(dest,doc.root.html);
		setTimes(dest,modified,modified);
	}
}

void check_chapter(SysTime modified, string markup) {
	check_chapter(modified, markup, dirName(markup));
}

void check_chapter(SysTime modified, string markup, string top) {
	import std.array : array;
	version(GNU) {
		import std.algorithm: findSplitBefore;
	} else {
		import std.algorithm.searching: findSplitBefore;
	}
	import std.path: pathSplitter;

	auto path = array(pathSplitter(markup));
	if(path.length != 3) return;
	if(path[1] != "markup") return;

	// pick stuff apart to analyze more closely
	//		location / "markup" / chapterxx.ext

	check_chapter(modified, to!string(path[0]),
								markup, top, 
								findSplitBefore(to!string(path[$-1]),".").expand);
}

Appender!(Update[]) pending_updates;
struct Upd8 {
	string location;
	int which;
}
bool[Upd8] updated;
bool[Upd8] one_before;
bool[Upd8] no_update;
// if A is the last chapter, and B is in one_before, add B to no_update
// if A is in no_update, don't update.
// that keeps us from constantly regenerating the chapter-before-last
// only do it if in one_before, because if not in one_before it actually needs updating

string only_location = null;

db.Story* place_story(string location, int which) {
	db.Story* story;
	if(location in stories) {
		story = &stories[location];
	} else {
		stories[location] = db.story(location);
		story = &stories[location];

		story.location = location;

		assert(story.location);
	}
	// new chapters at the end, we need to increase the story's number of
	// chapters, before performing ANY updates.
	// the database counts known chapters, so this is just a
	// temp cached number
	if(story.chapters <= which) {
		print("UPDATE STORY CHAPTERS",location,which,story.chapters);
		story.chapters = which + 1;
		story.dirty = true;
	}
	return story;
}

bool contents_exist_derp(string location, string category = "html") {
	import std.path: buildPath;
	auto contents = buildPath(category,location,"contents.html");
	return exists(contents);
}
alias contents_exist = memoize!contents_exist_derp;

void nope(string m) {
	print("not updating",m);
}

void check_chapter(SysTime modified,
									 string location,
									 string markup,
									 string markuploc,
									 string name,
									 string ext) {
	import std.string : isNumeric;
	version(GNU) {
		import std.algorithm: startsWith, endsWith, max;
	} else {
		import std.algorithm.searching: startsWith, endsWith;
		import std.algorithm.comparison: max;
	}
	import std.path: buildPath;
	import std.file: timeLastModified;

	if(!name.startsWith("chapter")) return nope("name dun' start with chapter");
	import std.stdio;
	string derp = name["chapter".length..name.length];
	if(!isNumeric(derp)) return nope("not numeric " ~ derp);
	int which = to!int(derp) - 1;
	if(!exists(location)) return nope("location not exists: " ~ location);

	// don't update if we're filtering by location...?
	if(only_location && (!location.endsWith(only_location))) return nope("not this location");

	auto key = Upd8(location,which);

	// if in updated, don't redo it, but if in one_before, promote to a normal chapter
	
	if(key in one_before) {
		one_before.remove(key);
	} else {
		if(key in updated) return;
	}

	// return true if updated
	if(!exists(markup)) return;

	auto story = place_story(location,which);

	// does this story have a "current" chapter?
	// are we only updating up to the current chapter?
	if(only_until_current &&
		 story.current_chapter > 0 &&
		 which > story.current_chapter) {
		return;
	}
	
	/* check the destination modified time */
	name = chapter_name(which); // ugh, chapter1 -> index

	string category = only_until_current ? "ready" : "html";

	smackdir(category);
	auto dest = buildPath(category,location);
	smackdir(dest);
	dest = buildPath(dest, name ~ ".html");

	// technically this is not needed, since git records the commit time
	modified = max(timeLastModified(markup),modified);

	if(// always update if dest is gone
		exists(dest) &&
		// can skip update if older than dest
		// EXCEPT if a side chapter
		modified <= timeLastModified(dest)) {

		if(!contents_exist(location,category)) {
			// update contents anyway
			place_story(location,which);
		}

		print("unmodified",location,which);
		//setTimes(dest,modified,modified);
		return;
	}

	print("checking",location,which,"for updates!",modified,dest,exists(dest));

	// note: do not try to shrink the story if fewer chapters are found.
	// unless the markup doesn't exist. We might not be processing the full
	// git log, and the highest chapter might not have updated this time.
	if(key !in updated) {
		updated[key] = true;
		pending_updates.emplacePut(story,modified,which,location,
															 markup,dest,name);
	}

	// also check side chapters for updates
	void check_side(int which, bool prev) {
		import std.format: format;
		auto key = Upd8(location,which);
		if(key in updated) return; // don't add a side chapter if we're updating normally.
		if(prev) {
			// if this chapter is the next one, always update
			// only the previous chapter being a side chapter should skip updates
			auto side = key in one_before;
			if(!(side is null)) return;
			one_before[key] = true;
		}
		print("SIDE CHAP",key);
		auto markup = buildPath(markuploc,"chapter%d.hish".format(which+1));
		SysTime modified;
		try {
			modified = timeLastModified(markup);
		} catch(Exception e) {
			try {
				markup = buildPath(markuploc,"chapter%d.txt".format(which+1));
				modified = timeLastModified(markup);
			} catch(Exception e) {
				return;
			}
		}
		auto name = chapter_name(which);
		auto dest = buildPath(category,location, name ~ ".html");
		updated[key] = true;
		//place_story(location,which); this will increase chapters beyond the last!
		pending_updates.emplacePut(story,modified,which,location,markup,dest,name);
	}
	if(which > 0) {
		check_side(which-1,true);
	} 
	check_side(which+1,false);
}

void check_chapters_for(string location) {
	import std.file: dirEntries,
		FileException, SpanMode, timeLastModified;
	import std.path: buildPath;

	string top = buildPath(location,"markup");
	foreach(string markup; dirEntries(top,SpanMode.shallow)) {
		print("checko",markup);
		check_chapter(timeLastModified(markup),markup,top);
	}
}

void check_git_log(string[] args) {
	// by default we just check the last commit
	// (run this in a post-commit hook)
	db.since_git((string last_version) {
			string since;
			if(args.length > 1) {
				since = args[1];
				if(since=="") since = null;
			} else if(last_version is null) {
				since = "HEAD";
			} else {
				since = last_version ~ "..HEAD";
			}
			print("since "~since);
			static import git;
			return git.parse_log(since,&check_chapter);
		});
}

void main(string[] args)
{
	import core.stdc.stdlib: getenv;
	import std.path: buildPath;

	while(!exists("code")) {
		import std.file: chdir;
		chdir("..");
	}

	if(getenv("onefile")) {
		htmlish.make(args[1],args[2],createDocument(import("template/chapter.xhtml")));
		return;
	}


	db.open();
	scope(exit) db.close();

	if(args.length == 2 && args[1] == "fixblobs") {
		import fixblobs: go;
		go();
		return;
	}
	
	if(auto location = getenv("edit")) {
		db.story(to!string(location)).edit();
		print("Edited");
		return;
	}
	update_last = (getenv("update_last") !is null);
	only_until_current = (getenv("ready") !is null);
	
	pending_updates = appender!(Update[]);
	if(getenv("story")) {
		only_location = to!string(getenv("story"));
		assert(only_location,"specify a story please!");
		check_chapters_for(only_location);
	} else if(auto location = getenv("finish")) {
		db.story(to!string(location)).finish();
		return;
	} else if(auto location = getenv("continue")) {
		db.story(to!string(location)).finish(false);
		return;
	} else {
		check_git_log(args);
	}
	if(stories.length) {
		smackdir("html");
		// now we can create all the outdirs
		foreach(outdir; stories.keys) {
			smackdir(buildPath("html",outdir));
		}
		print("stories",stories.values);
		foreach(story;stories.values) {
			print("DIRTY STORY",story,story.dirty);
			if(story.dirty) {
				story.update();
			}
		}

		foreach(ref update; pending_updates.data) {
			update.perform();
		}

		foreach(outdir; stories.keys) {
			if(outdir !in need_reindex) {
				print("didn't need to update",outdir);
				stories.remove(outdir);
			}
		}

		reindex("html",stories);
	} else {
		print("no stories updated");
	}
}
