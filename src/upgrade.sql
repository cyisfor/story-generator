attach 'old.sqlite' as old;

INSERT INTO categories SELECT id, category, timestamp as updated from old.categories;
insert into stories SELECT id, location, timestamp as created, timestamp as updated,
finished, chapters, ready, title, description, source FROM old.stories;
INSERT INTO categories SELECT id, category, timestamp as updated from old.categories;
