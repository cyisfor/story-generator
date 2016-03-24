static import backend = d2sqlite3;

version(GNU) {
  import std.algorithm: map, findSplit, findSplitBefore;
} else {
  import std.algorithm.iteration: map;
  import std.algorithm.searching: findSplit, findSplitBefore;
}
import std.datetime: SysTime, Clock;
import std.file: exists, readText;
import std.path: buildPath;
import std.stdio: writeln,readln,write;
import std.string: strip;
import std.array: join, appender;
struct Chapter {
  long id;
  string title;
  SysTime modified;
  string first_words;
  Database db;
  Story story;
  int which;
  void update(SysTime modified) {
	update(modified,which,title);
  }
  void update(SysTime modified, int which, string title) {
	this.which = which;
	db.update_chapter.inject(title,modified.toISOExtString(),which,story.id);
  }
}

SysTime parse_mytimestamp(string timestamp) {
  import std.conv: to;
  // sometimes year is negative, before 0 AD!
  auto when = timestamp.findSplit("/");
  auto derp = when[2].findSplitBefore(".");
  import std.datetime: Date, TimeOfDay, DateTime,
	UTC, TimeException;
  import core.exception: RangeError;
  import core.time: Duration, dur;
  try {
	const Date the_date = Date(to!short(when[0]),
							   to!ubyte(derp[0][0..2]),
							   to!ubyte(derp[0][2..4]));
	const TimeOfDay the_time = TimeOfDay.fromISOString
	  (derp[0][4..derp[0].length]);

	// sqlite dates cannot hold timezone info, so they're always relative to
	// UTC.
	// fromJulianDay would be nice, but floating point error :p

	// restricted access to private members,
	// because long compile times are a good thing!
	version(GNU) {
	  import core.time: FracSec;
	  const FracSec jeezus = FracSec.from!"hnsecs"(to!long(
									 100000000000 *
									 to!float(derp[1])));
	} else {
	  const Duration jeezus = dur!"hnsecs"(to!long(
												   100000000000 *
												   to!float(derp[1])));
	}
	return SysTime(DateTime(the_date,the_time),jeezus,UTC());
  } catch(TimeException e) {
	writeln("uhhh",timestamp);
	throw(e);
  } catch(RangeError e) {
	writeln("huhh?",timestamp);
	throw(e);
  }
}
  
Chapter to_chapter(backend.Row row) {
  import std.conv: to;
  struct temp {
	string id;
	string title;
	string modified;
	string first_words;
  }
  temp t = row.as!temp;

  Chapter ret = { id: to!long(t.id),
				  title: t.title,
				  modified: parse_mytimestamp(t.modified),
				  first_words: t.first_words
  };
  return ret;
}


struct Story {
  long id;
  string title;
  string description;
  int chapters;
  SysTime modified;
  Database db;
  string location;
  string url;
  Chapter opIndex(int which) {
	db.find_chapter.bindAll(id, which);
	auto rset = db.find_chapter.execute();
	if(rset.empty) {
	  db.insert_chapter.inject(which, id);
	  rset = db.find_chapter.execute();
	}
	Chapter ret = rset.front.to_chapter();
	db.find_chapter.reset();
	ret.db = db;
	ret.story = this;
	ret.which = which;
	return ret;
  }
  void update() {
	db.update_story.inject(id);
  }

  // SIGH
  import std.format: FormatSpec,formatValue;
  void toString(scope void delegate(const(char)[]) sink, FormatSpec!char fmt) const {
	sink("Story(");
	formatValue(sink,id,fmt);
	sink(":");
	formatValue(sink,title,fmt);
	sink(")");
  }
};

Story to_story(backend.Row row) {
  import std.conv: to;
  struct temp {
	long id;
	string title;
	string description;
	string modified;
	int chapters;
  }
  temp t = row.as!temp;
  Story ret = { id: t.id,
				title: t.title,
				description: t.description,
				modified: parse_mytimestamp(t.modified),
				chapters: t.chapters,
				url: "http://hellifiknow/",
  };
  return ret;
}

string readToDot() {
  auto lines = appender!string();
  for(;;) {
	string line = readln().strip();
	if(line == ".") break;
	lines.put(line);
	lines.put("\n");
  }
  return lines.data;
}

class Database {
  backend.Database db;
  backend.Statement update_desc;
  backend.Statement find_story;
  backend.Statement update_story;
  backend.Statement insert_story;
  backend.Statement find_chapter;
  backend.Statement update_chapter;
  backend.Statement insert_chapter;
  backend.Statement latest_stories;
  void close() {
	update_desc.finalize();
	find_story.finalize();
	update_story.finalize();
	insert_story.finalize();
	find_chapter.finalize();
	update_chapter.finalize();
	insert_chapter.finalize();
	latest_stories.finalize();
	db.close();
  }
  this() {
	immutable string modified_format =
	  "strftime('%Y/%m%d%H%M%f',modified)";
	immutable string story_fields = "id,title,description,"~modified_format~",location,(select count(id) from chapters where story = stories.id)";
	db = backend.Database("generate.sqlite");
	db.run(import("schema.sql"));
	import print: print;
	print("derp ran schema");
	update_desc = db.prepare("UPDATE stories SET title = COALESCE(?,title), description = COALESCE(?,description) WHERE id = ?");
	find_story = db.prepare("SELECT "~story_fields~" from stories where location = ?");
	// just some shortcut bookkeeping
	update_story = db.prepare(`UPDATE stories SET
modified = (SELECT MAX(modified) FROM chapters WHERE story = stories.id),
chapters = (select count(1) from chapters where story = stories.id)
WHERE id = ?`);
	insert_story = db.prepare("INSERT INTO stories (location,title,description) VALUES (?,?,?)");
	find_chapter = db.prepare("SELECT id,title, "~modified_format~", first_words FROM chapters WHERE story = ? AND which = ?");
	insert_chapter = db.prepare("INSERT INTO chapters (which, story) VALUES (?,?)");
	update_chapter = db.prepare("UPDATE chapters SET title = ?, modified = ? WHERE which = ? AND story = ?");
	latest_stories = db.prepare("SELECT "~story_fields~" FROM stories ORDER BY modified DESC LIMIT 100");
  }

  void check_for_desc(Story story) {
	string title = null;
	string description = null;
	if(exists(buildPath(story.location,"title"))) {
	  title = readText(buildPath(story.location,"title"));
	}
	if(exists(buildPath(story.location,"description"))) {
	  description = readText(buildPath(story.location,"description"));
	}
	if(title || description) {
	  update_desc.inject(title, description, story.id);
	}
  }

  Story story(string location) {
	find_story.bind(1,location);
	auto rows = find_story.execute();
	scope(exit) {
	  find_story.reset();
	}
	if(rows.empty) {
	  scope(exit) {
		insert_story.reset();
	  }
	  insert_story.bind(1,location);
	  writeln(location);
	  write("Title: ");
	  insert_story.bind(2,readln().strip());
	  writeln("Description: (end with a dot)");
	  insert_story.bind(3,readToDot());
	  insert_story.execute();
	  rows = find_story.execute();
	}
	Story story = rows.front.to_story();
	story.db = this;
	check_for_desc(story);
	return story;
  }

  auto latest() {
	return map!(to_story)
	  (latest_stories.execute());
  }
}

Database db;
void open() {
  db = new Database();
}

Story story(string location) {
  return db.story(location);
}

auto latest() {
  return db.latest();
}

void close() {
  db.close();
}


struct Transaction {
  bool committed;
  ~this() {
	if(!committed) 
	  db.db.rollback();
  }
  void commit() {
	db.db.commit();
	committed = true;
  }
};

Transaction transaction() {
  Transaction t;
  t.committed = false;
  db.db.begin();
  return t;
}

unittest {
  db.close(); // avoid memory leak error
  import std.stdio;
  writeln("derpaherp");
}
