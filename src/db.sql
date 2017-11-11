CREATE TABLE IF NOT EXISTS categories (
			 id INTEGER PRIMARY KEY,
			 category TEXT NOT NULL UNIQUE,
			 updated INTEGER NOT NULL DEFAULT 0);

CREATE TABLE IF NOT EXISTS committing (
-- NULLs okay
-- we can't use until/since as a primary key, since they can't be NULL then b/c sqlite sux
			 until INTEGER,
			 since BLOB);

CREATE TABLE IF NOT EXISTS stories (
			 id INTEGER PRIMARY KEY,
			 location TEXT NOT NULL UNIQUE,
			 created INTEGER NOT NULL,
			 updated INTEGER NOT NULL,
			 finished BOOLEAN NOT NULL DEFAULT FALSE,
			 -- these are how many chapters the story had the LAST
			 -- time its contents were calculated. COUNT(1) FROM chapters for current count
			 chapters INTEGER NOT NULL DEFAULT 0,
			 -- this is the latest chapter that is "ready"
			 -- i.e. no longer in draft.
			 -- TODO: readiness levels?
			 ready INTEGER,
			 title TEXT,
			 description TEXT,
			 source TEXT);

CREATE TABLE IF NOT EXISTS chapters (
			 story INTEGER NOT NULL REFERENCES stories(id) ON DELETE RESTRICT ON UPDATE CASCADE,
			 chapter INTEGER NOT NULL,
			 created INTEGER NOT NULL,
			 updated INTEGER NOT NULL,
			 seen INTEGER NOT NULL,
			 -- seen = acts like it was updated, but was marked by an auxiliary process
			 -- greatest(updated,seen) -> actual
			 title TEXT,
			 PRIMARY KEY(story,chapter)) WITHOUT ROWID;

CREATE TABLE IF NOT EXISTS cool_xml_tags (
	tag TEXT PRIMARY KEY) WITHOUT ROWID;

CREATE TABLE IF NOT EXISTS censored_stories (
	story INTEGER PRIMARY KEY NOT NULL REFERENCES stories(id)
	ON DELETE CASCADE ON UPDATE CASCADE);
