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

import html: createDocument;

import print: print;
import std.conv: to;
import std.datetime : SysTime;
import std.algorithm: move;
import std.functional: memoize;

db.Story[string] stories;

void delegate(string,string) default_make_chapter;

static this() {
  static import htmlish;

  default_make_chapter = htmlish.make
    (createDocument(import("template/chapter.xhtml")));
}

void smackdir(string p) {
  import std.file: mkdir;
  try {
    mkdir(p);
  } catch {}
}


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
        return default_make_chapter;
      }
    }
    auto make = herpaderp();

    print("creating chapter",which,markup,story);
    auto chapter = story.get_chapter(which);

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


    chapter.update(modified,
                   chapter.which,
                   title);

    print("writing",location, which,dest);
    write(dest,doc.root.html);
    setTimes(dest,modified,modified);
  }
}

void check_chapter(SysTime modified, string markup) {
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
  //    location / "markup" / chapterxx.ext

  check_chapter(modified, to!string(path[0]),
                markup,
                findSplitBefore(to!string(path[path.length-1]),".").expand);
}

Appender!(Update[]) pending_updates;
struct Upd8 {
  string location;
  int which;
}
bool[Upd8] updated;
string only_location = null;

void place_story(string location, int which) {
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
    story.chapters = which+1;
    story.dirty = true;
  }
}

bool contents_exist_derp(string location) {
  auto contents = buildPath("html",location,"contents.html");
  return exists(contents);
}
alias contents_exist = memoize!contents_exist_derp;

void check_chapter(SysTime modified,
                   string location,
                   string markup,
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

  if(!name.startsWith("chapter")) return;

  string derp = name["chapter".length..name.length];
  if(!isNumeric(derp)) return;
  int which = to!int(derp) - 1;
  if(!exists(location)) return;

  // now we're sure it's a chapter.

  // don't update if we're filtering by location...?
  if(only_location && (!location.endsWith(only_location))) return;

  void checked_chapter(int which, string markup) {
    if(!exists(markup)) return;

    // only after we're sure we have a chapter that hasn't been queued
    // for updating.
    
    /* check the destination modified time */
    name = chapter_name(which); // ugh, chapter1 -> index

    auto dest = buildPath("html",location, name ~ ".html");

    // technically this is not needed, since git records the commit time
    modified = max(timeLastModified(markup),modified);

    if(// always update if dest is gone
       exists(dest) &&
       // can skip update if older than dest
       modified <= timeLastModified(dest)) {
      if(!contents_exist(location)) {
        // update contents anyway
        place_story(location,which);
      }
       // always update if dest is gone
      print("unmodified",location,which);
      //setTimes(dest,modified,modified);
      return;
    }

    print("checking",location,which,"for updates!");

    place_story(location,which);

    // note: do not try to shrink the story if fewer chapters are found.
    // unless the markup doesn't exist. We might not be processing the full
    // git log, and the highest chapter might not have updated this time.
    pending_updates.emplacePut(story,modified,which,location,
                               markup,dest,name);
  }

  void checked_chapter_side(int which) {
    auto key = Upd8(location,which);
    if(key !in updated) {
      // since this isn't chapter 0, (markup provided by git)
      // adjust stuff to aim at the new chapter
      updated[key] = true;
      checked_chapter(which,
                      buildPath(location,
                                "markup",
                                "chapter" ~ to!string(which+1) ~ ext));
    }
  }
  
  // don't add updates to a chapter twice, if modified twice in the logs
  // or 2, 3 in logs (adding side chapters 1,3, and 2,4)

  auto key = Upd8(location,which);
  if(key !in updated) {
    updated[key] = true;
    checked_chapter(which,markup);
  }
  if(which>0)
    checked_chapter_side(which-1);
  checked_chapter_side(which+1);
}

void main(string[] args)
{
  import core.stdc.stdlib: getenv;
  import std.path: buildPath;

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
    assert(only_location,"specify a story please!");
    check_chapters_for(only_location);
  } else {
    check_git_log(args);
  }
  if(stories.length) {
    smackdir("html");
    // now we can create all the outdirs
    foreach(outdir; stories.keys) {
      smackdir(buildPath("html",outdir));
    }
    foreach(ref update; pending_updates.data) {
      update.perform();
    }
    foreach(story;stories.values) {
      story.update();
    }
    reindex("html",stories);
    print("dunZ");
  } else {
    print("no stories updated");
  }
}

void check_chapters_for(string location) {
  import std.file: dirEntries,
    FileException, SpanMode, timeLastModified;
  import std.path: buildPath;

  string top = buildPath(location,"markup");
  foreach(string markup; dirEntries(top,SpanMode.shallow)) {
    check_chapter(timeLastModified(markup),markup);
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
