attach 'generate.sqlite' as o;
with descs AS (SELECT location,description,title FROM o.stories)
UPDATE stories SET
description = (SELECT description FROM descs WHERE descs.location = stories.location),
title = (SELECT title FROM descs WHERE descs.location = stories.location);
