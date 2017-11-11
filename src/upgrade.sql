attach 'old.sqlite' as old;
.read src/db.sql
INSERT INTO categories SELECT id, category, timestamp as updated from old.categories;
insert into stories SELECT id, location, timestamp as created, timestamp as updated,
finished, chapters, ready, title, description, source FROM old.stories;
INSERT INTO chapters SELECT story, chapter,
timestamp as updated, timestamp + random() % 10000 AS created, timestamp as seen,
title FROM old.chapters;
INSERT INTO cool_xml_tags SELECT tag FROM old.cool_xml_tags;
INSERT INTO censored_stories SELECT story FROM old.censored_stories;

.read src/indexes.sql
