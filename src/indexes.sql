CREATE INDEX IF NOT EXISTS storytime ON stories(timestamp);
-- we can do this... right?
CREATE INDEX IF NOT EXISTS bystory ON chapters(story);
CREATE INDEX IF NOT EXISTS chaptertime ON chapters(updated);
CREATE INDEX IF NOT EXISTS storytime ON stories(updated);
CREATE UNIQUE INDEX IF NOT EXISTS storycreated ON stories(created);
CREATE UNIQUE INDEX IF NOT EXISTS chaptercreated ON chapters(created);
