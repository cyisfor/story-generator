CREATE TABLE IF NOT EXISTS categories (
			 id INTEGER PRIMARY KEY,
			 category TEXT NOT NULL UNIQUE,
			 timestamp INTEGER NOT NULL DEFAULT 0);

CREATE TABLE IF NOT EXISTS committing (
			 until INTEGER NOT NULL,
			 since BLOB NOT NULL,
			 PRIMARY KEY(until,since)) WITHOUT ROWID;

CREATE TABLE IF NOT EXISTS stories (
			 id INTEGER PRIMARY KEY,
			 location TEXT NOT NULL UNIQUE,
			 timestamp INTEGER NOT NULL,
			 finished BOOLEAN NOT NULL DEFAULT FALSE,
			 -- these are how many chapters the story had the LAST
			 -- time its contents were written. COUNT(1) FROM chapters for accurate count
			 chapters INTEGER NOT NULL DEFAULT 0,
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
CREATE INDEX IF NOT EXISTS bystory ON chapters(story);
CREATE INDEX IF NOT EXISTS chaptertime ON chapters(timestamp);

CREATE TABLE IF NOT EXISTS cool_xml_tags (
	tag TEXT PRIMARY KEY) WITHOUT ROWID;
