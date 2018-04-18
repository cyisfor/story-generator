derp
SAW_CHAPTER_UPDATE_STORY
UPDATE stories SET
  updated = MAX(updated,?1)
WHERE
  id = ?2 AND
	?3 OR
	(CASE WHEN ready IS NULL THEN
	  (chapters > ?4)
	ELSE
	  ready >= ?4);
RECENT_CHAPTERS
SELECT story, chapter, stories.title, chapters.title,
       location,
			 chapters.updated
FROM chapters
  INNER JOIN stories on stories.id = chapters.story
WHERE 
-- not (only censored and in censored_stories)
  NOT (?1 AND story IN (select story from censored_stories)) 
  AND (
    -- all ready, or this one ready, or not last chapter
    ?2
    OR
		chapter <= COALESCE(stories.ready, stories.chapters - 1)
		OR
		-- always stories with one chapter are ready
    1 = stories.chapters
  )
ORDER BY chapters.updated DESC LIMIT ?3);
FOR_ONLY_STORY
SELECT location,ready,chapters,updated FROM stories WHERE id = ?1
-- not only censored, and this story is censored
  AND NOT(?2 AND id IN (SELECT story FROM censored_stories));
FOR_STORIES
SELECT id,location,ready,chapters,updated FROM stories WHERE
  updated AND updated > ? ORDER BY updated;
FOR_UNDESCRIBED_STORIES
SELECT id,COALESCE(title,location),description,source
FROM stories WHERE 
  title IS NULL OR
	title = '' OR 
  description IS NULL OR
	description = '';
FOR_CHAPTERS
SELECT chapter,updated FROM chapters
WHERE 
  story = ?1 AND
	(updated > ?2 OR seen > ?2) AND 
	(?3 OR 
		chapter <= (SELECT COALESCE(ready, chapters-1)
		  FROM stories WHERE id = chapters.story));
