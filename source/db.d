static import backend = d2sqlite3;
import print: print;
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
  @disable this(this);
  long id;
  string title;
  SysTime modified;
  string first_words;
  Database db;
  Story story;
  int which;
  void remove() {
	story.remove(which);
	this = Chapter.init;
  }
  void update(SysTime modified) {
	update(modified,which,title);
  }
  void update(SysTime modified, int which, string title) {
	this.which = which;
	import std.stdio;
	if(title is null) { title = "???"; }
	db.update_chapter.inject(title,modified.toISOExtString(),which,story.id);
  }
};

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

Chapter to_chapter(backend.Row row,
				   Database db, Story story, int which) {
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
				  first_words: t.first_words,
				  db: db,
				  story: story,
				  which: which
  };
  return ret;
}


class Story {

  long id;
  string title;
  string description;
  int chapters;
  SysTime modified;
  Database db;
  string location;
  Chapter[int] cache;
  alias opIndex = get_chapter!false;

  static struct Params {
	long id;
	string title;
	string description;
	string modified;
	string location;
	int chapters;
  };
  this(Params p, Database db) {
	location = p.location;
	this.db = db;
	id = p.id;
	title = p.title;
	description = p.description;
	modified = parse_mytimestamp(p.modified);
	chapters = p.chapters;
	check_for_desc();
  }

  Chapter* get_chapter(bool create = true)(int which) {
	assert(id>=0);
	db.find_chapter.bindAll(id, which);
	auto rset = db.find_chapter.execute();
	if(rset.empty) {
	  db.insert_chapter.inject(which, id);
	  db.find_chapter.reset();
	  rset = db.find_chapter.execute();
	}
	cache[which] = rset.front.to_chapter(db,this,which);
	db.find_chapter.reset();

	return &cache[which];
  }

  void remove(int which) {
	db.delete_chapter.inject(which,id);
	update();
  }

  void update() {
	db.update_story.inject(id);
	db.num_story_chapters.bindAll(id);

	chapters = db.num_story_chapters.execute().front.peek!int(0);
	db.num_story_chapters.reset();
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

  void check_for_desc() {
	bool dirty = false;
	if(exists(buildPath(this.location,"title"))) {
	  title = readText(buildPath(this.location,"title"));
	  dirty = true;
	}
	if(exists(buildPath(this.location,"description"))) {
	  description = readText(buildPath(this.location,"description"));
	  dirty = true;
	}
	if(dirty) {
	  db.update_desc.inject(title, description, id);
	}
  }

  void edit() {
	import std.stdio: writeln;
	writeln("Title: ",title);
	writeln(description);
	db.edit_story.bind(1,id);
	db.get_info(db.edit_story);
  }
};

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

struct Derp {
  string name;
  string stmt;
}

immutable string modified_format =
  "strftime('%Y/%m%d%H%M%f',modified)";
immutable string story_fields = "id,title,description,"~modified_format~",location,chapters";


immutable Derp[] statements = [
  {q{update_desc},
   "UPDATE stories SET title = COALESCE(?,title), description = COALESCE(?,description) WHERE id = ?"},

  {q{num_story_chapters},
   "SELECT COUNT(1) FROM chapters WHERE story = ?"},

  {q{find_story},
   "SELECT "~story_fields~" from stories where location = ?"},

  // just some shortcut bookkeeping
  {q{update_story},
   `UPDATE stories SET
modified = (SELECT MAX(modified) FROM chapters WHERE story = stories.id),
chapters = (select count(1) from chapters where story = stories.id)
WHERE id = ?`},

  {q{insert_story},
   "INSERT INTO stories (location,title,description) VALUES (?,?,?)"},

  {q{edit_story},
   "UPDATE stories SET title = ?2, description = ?3 WHERE id = ?1"},

  {q{find_chapter},
   "SELECT id,title, "~modified_format~", first_words FROM chapters WHERE story = ? AND which = ?"},

  {q{insert_chapter},
   "INSERT INTO chapters (which, story) VALUES (?,?)"},

  {q{delete_chapter},
   "DELETE FROM chapters WHERE which = ? AND story = ?"},

  {q{update_chapter},
   "UPDATE chapters SET title = ?, modified = ? WHERE which = ? AND story = ?"},

  {q{latest_stories},
   "SELECT "~story_fields~" FROM stories ORDER BY modified DESC LIMIT 100"},
];

string declare_statements() {
  string ret;
  foreach(stmt; statements) {
	ret ~= q{backend.Statement }~stmt.name~";\n";
  }
  return ret;
}

string finalize_statements() {
  string ret;
  foreach(stmt; statements) {
	ret ~= stmt.name~".finalize();\n";
  }
  return ret;
}

string initialize_statements() {
  import std.format: format;
  string ret;
  foreach(stmt; statements) {
	ret ~= format(q{
		%s = db.prepare("%s");
	  },stmt.name,stmt.stmt);
  }
  return ret;
}

struct OneShot {
  backend.Statement stmt;
  auto opCall() {
	auto ret = stmt.execute();
	stmt.reset();
	return ret;
  }
}

class Database {
  backend.Database db;
  mixin(declare_statements());

  OneShot begin;
  OneShot commit;
  OneShot rollback;

  void close() {
	mixin(finalize_statements());
	db.close();
  }
  this() {
	db = backend.Database("generate.sqlite");
	db.run(import("schema.sql"));
	import print: print;
	mixin(initialize_statements());
	begin.stmt = db.prepare("BEGIN");
	commit.stmt = db.prepare("COMMIT");
	rollback.stmt = db.prepare("ROLLBACK");
  }

  void get_info(ref backend.Statement stmt) {
	write("Title: ");
	stmt.bind(2,readln().strip());
	writeln("Description: (end with a dot)");
	stmt.bind(3,readToDot());
    stmt.execute();
	stmt.reset();
  }

  Story story(string location) {
	find_story.bind(1,location);
	scope(exit) find_story.reset();
	auto rows = find_story.execute();
	if(rows.empty) {
	  insert_story.bind(1,location);
	  get_info(insert_story);
	  rows = find_story.execute();
	}
	return new Story(rows.front.as!(Story.Params),this);
  }

  auto latest() {
	return map!((auto ref row) => new Story(row.as!(Story.Params),this))
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
  db = null;
}


struct Transaction {
  bool committed;
  ~this() {
	if(!committed)
	  db.rollback();
  }
  void commit() {
	db.commit();
	committed = true;
  }
};

Transaction transaction() {
  Transaction t;
  t.committed = false;
  db.begin();
  return t;
}

unittest {
  if(db !is null) db.close(); // avoid memory leak error
  import std.stdio;
  writeln("derpaherp");
}
