CREATE TABLE IF NOT EXISTS stories (
id INTEGER PRIMARY KEY,
location TEXT UNIQUE NOT NULL,
modified TIMESTAMP DEFAULT (julianday(0)) NOT NULL,
title TEXT NOT NULL,
description TEXT NOT NULL);

CREATE TABLE IF NOT EXISTS chapters (
id INTEGER PRIMARY KEY,
story INTEGER REFERENCES stories(id) ON DELETE CASCADE ON UPDATE CASCADE NOT NULL,
modified TIMESTAMP DEFAULT (julianday(0)) NOT NULL,
which INTEGER NOT NULL,
title TEXT,
first_words TEXT,
UNIQUE(story,which));

CREATE INDEX IF NOT EXISTS bystory ON chapters(story);
CREATE INDEX IF NOT EXISTS storymodified ON stories(modified);
CREATE INDEX IF NOT EXISTS chapmodified ON chapters(modified);
