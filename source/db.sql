CREATE TABLE IF NOT EXISTS commits (
			 oid BLOB NOT NULL,
			 timestamp INTEGER NOT NULL,
			 committed BOOL NOT NULL DEFAULT 0,
			 PRIMARY KEY(oid,timestamp)) WITHOUT ROWID;

-- we can do this... right?
CREATE UNIQUE INDEX IF NOT EXISTS committime ON commits(timestamp);

CREATE TABLE IF NOT EXISTS stories (
			 id INTEGER PRIMARY KEY,
			 location TEXT NOT NULL UNIQUE,
			 timestamp INTEGER NOT NULL DEFAULT 0,
			 title TEXT,
			 description TEXT,
			 source TEXT);
			 
CREATE INDEX IF NOT EXISTS storytime ON stories(timestamp);

CREATE TABLE IF NOT EXISTS chapters (
			 story INTEGER NOT NULL REFERENCES stories(id) ON DELETE RESTRICT ON UPDATE CASCADE,
			 chapter INTEGER NOT NULL,
			 timestamp INTEGER NOT NULL,
			 title TEXT,
			 PRIMARY KEY(story,chapter)) WITHOUT ROWID;

-- we can do this... right?
CREATE UNIQUE INDEX IF NOT EXISTS bystory ON chapters(story);
CREATE INDEX IF NOT EXISTS chaptertime ON chapters(timestamp);
