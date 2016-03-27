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

import print: print;
import std.conv: to;
import std.datetime : SysTime;
import std.algorithm: move;

db.Story[string] stories;

void delegate(string,string) default_make_chapter;

static this() {
	import htmlderp: createDocument;
	static import htmlish;
		
	default_make_chapter = htmlish.make
	  (createDocument(import("template/chapter.xhtml")));
}

struct Update {
	@disable this(this);
	db.Story* story;
	SysTime modified;
	int which;
	string location;
	bool is_hish;
	void perform() {
	auto transaction = db.transaction();
	scope(success) transaction.commit();
	import std.file: setTimes;
	import htmlderp: querySelector, createDocument;
	import makers;
	static import nada = makers.birthverse;
	static import nada2 = makers.littlepip;
	import std.file: write, mkdir, timeLastModified, readText;
	import std.path: buildPath;

	void smackdir(string p) {
		try {
		mkdir(p);
		} catch {}
	}

	string ext;
	if( is_hish ) {
		ext = ".hish";
	} else {
		ext = ".txt";
	}

	auto name = chapter_name(which);
	auto markup = buildPath(location,"markup","chapter" ~ to!string(which+1) ~ ext);
	if(!exists(markup)) {
		story.remove(which);
		return;
	}

	// find a better place to put stuff so it doesn't scram the source directory
	string basedir = "tmp";
	smackdir(basedir);
	basedir = buildPath(basedir,"base");
	smackdir(basedir);
	basedir = buildPath(basedir,location);
	smackdir(basedir);
	auto herpaderp() {
		auto box = location in makers.make;
		if(box) {
		print("found maker at ",location);
		return *box;
		} else {
		return default_make_chapter;
		}
	}
	auto make = herpaderp();

	print("creating cahpter",which,markup);
	auto chapter = story.get_chapter(which);
	string outdir = buildPath("html",location);
	auto dest = buildPath(outdir,chapter_name(chapter.which) ~ ".html");

	SysTime new_modified = timeLastModified(markup);
	if(new_modified > chapter.modified) {
		modified = new_modified;
	}
	if(modified <= chapter.modified) {
		if(exists(dest)) {
		print("unmodified",story.title,chapter.which);
		//setTimes(dest,modified,modified);
		return;
		}
		// always update if dest is gone
	}

	auto base = buildPath(basedir,name ~ ".html");
	make(markup,base);
	auto doc = createDocument
		(readText(buildPath(basedir,
							chapter_name(chapter.which) ~
							".html")));

	auto head = querySelector(doc,"head");
	auto links = doc.querySelector("#links");
	if(links is null) {
		links = doc.createElement("div", querySelector(doc,"body"));
		links.attr("class","links");
	}
	bool didlinks = false;
	void dolink(string href, string rel, string title) {
		auto link = doc.createElement("link",head);
		link.attr("rel",rel);
		link.attr("href",href);
		if( didlinks ) {
		links.appendText(" ");
		} else {
		didlinks = true;
		}
		link = doc.createElement("a",links);
		link.attr("href",href);
		link.appendText(title);
	}
	if( chapter.which > 0 ) {
		dolink(chapter_name(chapter.which-1)~".html", "prev", "Previous");
	}
	if( chapter.which + 1 < story.chapters ) {
		dolink(chapter_name(chapter.which+1)~".html", "next", "Next");
	}

	if(auto box = location in makers.chapter) {
		(*box)(doc);
	}
	chapter.update(modified,
					chapter.which,
					to!string(querySelector(doc,"title").html));

	smackdir("html");
	smackdir(outdir);
	print("writing to ",outdir,"/",chapter_name(chapter.which));
	print(doc);
	print(doc.root.document_);
	write(dest,doc.root.html);

	setTimes(dest,modified,modified);
	string chapterTitle = to!string(querySelector(doc,"title").text);
	(*story)[which].update(modified, which, chapterTitle);
	}
}

void check_chapter(SysTime modified, string spath) {
	import std.array : array;
	version(GNU) {
	import std.algorithm: findSplitBefore;
	} else {
	import std.algorithm.searching: findSplitBefore;
	}
	import std.path: pathSplitter;

	auto path = array(pathSplitter(spath));
	if(path.length != 3) return;
	//	location / markup / chapterxx.xx
	if(path[1] != "markup") return;

	check_chapter(modified, to!string(path[0]),
				findSplitBefore(to!string(path[path.length-1]),".").expand);
}

Appender!(Update[]) pending_updates;
bool[string] updated;
string only_location = null;

void check_chapter(SysTime modified,
					string location,
					string name,
					string ext) {
  import std.string : isNumeric;
  version(GNU) {
	import std.algorithm: startsWith, endsWith;
  } else {
	import std.algorithm.searching: startsWith, endsWith;
  }

  if(!name.startsWith("chapter")) return;

	bool is_hish = ext == ".hish";
	if(!(ext == ".txt" || is_hish)) return;

	string derp = name["chapter".length..name.length];
	if(!isNumeric(derp)) return;
	int which = to!int(derp) - 1;

	if(!exists(location)) return;
	if(only_location && (!location.endsWith(only_location))) return;

	for(int i=which-1;i<=which+1;++i) {
	string key = location ~ "/" ~ to!string(which);
	if(key in updated) continue;
	updated[key] = true;
	print("checking",location,which,"for updates!");
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
	if(story.chapters <= which) {
		story.chapters = which+1;
	}
	// note: do not try to shrink the story if fewer chapters are found.
	// unless the markup doesn't exist. We might not be processing the full
	// git log, and the highest chapter might not have updated this time.

	pending_updates.emplacePut(story,modified,which,location,is_hish);
	}
}

void perform_updates() {
	foreach(ref update; pending_updates.data) {
	update.perform();
	}
}

void main(string[] args)
{
	import core.stdc.stdlib: getenv;
	while(!exists("code")) {
	import std.file: chdir;
	chdir("..");
	}
	db.open();
	scope(exit) db.close();

	if(auto location = getenv("edit")) {
	  db.story(to!string(location)).edit();
	  print("Edited");
	  return;
	}

	pending_updates = appender!(Update[]);
	if(getenv("story")) {
	only_location = to!string(getenv("story"));
	if(getenv("regenerate")) {
		assert(only_location,"specify a story please!");
		check_chapters_for(only_location);
		perform_updates();
		return;
	}
	}
	check_git_log(args);
	perform_updates();
	reindex("html",stories);
	print("dunZ");
}

void check_chapters_for(string location) {
	import std.file: dirEntries,
			FileException, SpanMode, timeLastModified;
	import std.path: buildPath;

	string top = buildPath(location,"markup");
	foreach(string spath; dirEntries(top,SpanMode.shallow)) {
	check_chapter(timeLastModified(spath),spath);
	}
}

void check_git_log(string[] args) {
		// by default we just check the last commit
	// (run this in a post-commit hook)
	string since = "HEAD~1..HEAD";
	if(args.length > 1) {
	since = args[1];
	if(since=="") since = null;
	}
	print("since "~since);
	static import git;
	git.parse_log(since,&check_chapter);
}
