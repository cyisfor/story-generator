CREATE TABLE IF NOT EXISTS stories (
id INTEGER PRIMARY KEY,
location TEXT UNIQUE NOT NULL,
chapters INTEGER NOT NULL DEFAULT 0,
modified TIMESTAMP DEFAULT (julianday(0)) NOT NULL,
title TEXT NOT NULL,
description TEXT NOT NULL,
finished BOOL NOT NULL DEFAULT FALSE,
current_chapter INT NOT NULL DEFAULT -1);

CREATE TABLE IF NOT EXISTS chapters (
id INTEGER PRIMARY KEY,
story INTEGER REFERENCES stories(id) ON DELETE CASCADE ON UPDATE CASCADE NOT NULL,
modified TIMESTAMP DEFAULT (julianday(0)) NOT NULL,
which INTEGER NOT NULL,
title TEXT,
first_words TEXT,
has_next BOOLEAN DEFAULT 0 NOT NULL,
UNIQUE(story,which));

CREATE INDEX IF NOT EXISTS bystory ON chapters(story);
CREATE INDEX IF NOT EXISTS storymodified ON stories(modified);
CREATE INDEX IF NOT EXISTS chapmodified ON chapters(modified);

CREATE TABLE IF NOT EXISTS git (
id INTEGER PRIMARY KEY,
version TEXT UNIQUE);
