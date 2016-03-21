static import backend = d2sqlite3;

import std.algorithm.iteration: map;
import std.datetime: SysTime, Clock;
import std.file: exists, readText;
import std.path: buildPath;
import std.stdio: writeln,readln,write;
import std.string: strip;
import std.array: join, appender;
struct Chapter {
  string title;
  SysTime modified;
  string first_words;
  Database db;
  Story story;
  int which;
  void update(SysTime modified, int which, string title) {
	this.which = which;
	db.update_chapter.bindAll(title,modified.toUnixTime,which,story.id);
	db.update_chapter.execute();
  }
}

Chapter fucking_hell(backend.Row row) {
  struct temp {
	string title;
	long modified;
	string first_words;
  }
  temp t = row.as!temp;
  Chapter ret = { title: t.title,
				  modified: SysTime.fromUnixTime(t.modified),
				  first_words: t.first_words
  };
  return ret;
}


struct Story {
  long id;
  string title;
  string description;
  int chapters;
  Database db;
  string location;
  string url;
  Chapter opIndex(int which) {
	db.find_chapter.bindAll(id, which);
	auto rset = db.find_chapter.execute();
	if(rset.empty) {
	  db.insert_chapter.bindAll(which, id, Clock.currTime.toUnixTime);
	  db.insert_chapter.execute();
	  db.insert_chapter.reset();
	  rset = db.find_chapter.execute();
	}
	Chapter ret = rset.front.fucking_hell();
	db.find_chapter.reset();
	ret.db = db;
	ret.story = this;
	ret.which = which;
	return ret;
  }
};

Story to_story(backend.Row row) {
  struct temp {
	long id;
	string title;
	string description;
	int chapters;
  }
  temp t = row.as!temp;
  Story ret = { id: t.id,
				title: t.title,
				description: t.description,
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
  backend.Statement insert_story;
  backend.Statement find_chapter;
  backend.Statement update_chapter;
  backend.Statement insert_chapter;
  backend.Statement latest_stories;
  void close() {
	update_desc.finalize();
	find_story.finalize();
	insert_story.finalize();
	find_chapter.finalize();
	update_chapter.finalize();
	insert_chapter.finalize();
	latest_stories.finalize();
	db.close();
  }
  this() {
	immutable string story_fields = "id,title,description,location,(select count(id) from chapters where story = stories.id)";
	db = backend.Database("generate.sqlite");
	db.run(import("schema.sql"));
	update_desc = db.prepare("UPDATE stories SET title = COALESCE(?,title), description = COALESCE(?,description) WHERE id = ?");
	find_story = db.prepare("SELECT "~story_fields~" from stories where location = ?");
	insert_story = db.prepare("INSERT INTO stories (location,title,description,modified) VALUES (?,?,?,?)");
	find_chapter = db.prepare("SELECT title, modified, first_words FROM chapters WHERE story = ? AND which = ?");
	insert_chapter = db.prepare("INSERT INTO chapters (which, story, modified) VALUES (?,?,?)");
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
	  update_desc.bindAll(title, description, story.id);
	  update_desc.execute();
	  update_desc.reset();
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
	  insert_story.bind(4, Clock.currTime.toUnixTime);
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
static this() {
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
