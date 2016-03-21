import htmlderp: querySelector;
static import git;
static import db;
import reindex: reindex, chapter_name;

import makers;
static import nada = makers.birthverse;
static import nada2 = makers.littlepip;
static import htmlish;

import html: Document, createDocument;

version(GNU) {
  import std.algorithm: endsWith;
} else {
  import std.algorithm.searching: endsWith;
}
import std.stdio: writeln, writefln;
import std.file: write, mkdir, timeLastModified, readText, exists, chdir,
  FileException;
import std.path: buildPath;
import std.conv: to;
import std.datetime : SysTime;

import core.memory: GC;

db.Story[string] stories;

void smackdir(string p) {
  try {
	mkdir(p);
  } catch {
	
  }
}

struct Update {
  SysTime modified;
  int which;
  string location;
  bool is_hish;
  void update() {
	db.Story story;
	if(location in stories) {
	  story = stories[location];
	} else {
	  story = db.story(location);
	  stories[location] = story;
	  assert(stories.length > 0);
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
		writeln("found maker at ",location);
		return *box;
	  } else {
		return &htmlish.make;
	  }
	}
	auto make = herpaderp();

	string ext;
	if( is_hish ) {
	  ext = ".hish";
	} else {
	  ext = ".txt";
	}
	
	auto name = chapter_name(which);
	auto markup = buildPath(location,"markup","chapter" ~ to!string(which+1) ~ ext);

	auto chapter = story[which];

	try {
	  if(timeLastModified(markup) <= chapter.modified) {
		writeln("unmodified");
		return;
	  }
	} catch(FileException) {
	  writeln("markup not found ",markup);
	  return;
	}
	
	auto base = buildPath(basedir,name ~ ".html");
	make(markup,base);
	Document doc = createDocument
	  (readText(buildPath(basedir,
						  chapter_name(chapter.which) ~
						  ".html")));

	auto head = querySelector(doc,"head");
	auto links = querySelector(doc,"#links");
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
	  dolink(chapter_name(chapter.which-1), "prev", "Previous");
	}
	if( chapter.which + 1 < story.chapters ) {
	  dolink(chapter_name(chapter.which+1), "next", "Next");
	}

	auto box = location in makers.chapter;
	if(box) {
	  *box(doc);
	}
	chapter.update(modified, which, to!string(querySelector(doc,"title").html));

	smackdir("html");
	string outdir = buildPath("html",location);
	smackdir(outdir);
	writeln("writing to ",outdir,"/",chapter_name(chapter.which));
	write(buildPath(outdir,
					chapter_name(chapter.which) ~
					".html"),doc.root.html);
	
	string chapterTitle = to!string(querySelector(doc,"title").text);
	story[which].update(modified, which, chapterTitle);
  }
}

Update[string] queue;
string only_location = null;

void check_chapter(SysTime modified, int which, string location, bool is_hish) {
  if(!exists(location)) return;
  if(only_location && (!location.endsWith(only_location))) return;

  string key = location ~ "/" ~ to!string(which);
  if(key in queue) return;
  queue[key] = Update(modified,which,location,is_hish);
}


void main(string[] args)
{
  scope(exit) {
	db.close();
  }
  string since = "HEAD~20..HEAD";
  while(!exists(".git")) {
	writeln("boing");
	chdir("..");
  }
  if(args.length > 1) {
	since = args[1];
	if(since=="") since = null;
  }
  writeln("since "~since);
  git.parse_log(since,&check_chapter);
  foreach(update; queue.values) {
	writeln("checking ",update.location," ",update.modified);
	update.update();
  }
  reindex(".",stories);
}
