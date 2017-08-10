CREATE TABLE IF NOT EXISTS commits (
			 oid BLOB NOT NULL,
			 timestamp INTEGER NOT NULL,
			 PRIMARY KEY(oid,timestamp)) WITHOUT ROWID;

-- we can do this... right?
CREATE UNIQUE INDEX IF NOT EXISTS bytimestamp ON commits(timestamp);

CREATE TABLE IF NOT EXISTS stories (
			 id INTEGER PRIMARY KEY,
			 location TEXT NOT NULL UNIQUE,
			 timestamp INTEGER NOT NULL);

CREATE TABLE IF NOT EXISTS chapters (
			 story INTEGER NOT NULL REFERENCES stories(id) ON DELETE RESTRICT ON UPDATE CASCADE,
			 chapter INTEGER NOT NULL,
			 timestamp INTEGER NOT NULL,
			 PRIMARY KEY(story,chapter)) WITHOUT ROWID;


			 
