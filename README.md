Given a git repository that has &lt;storyname&gt;/markup/chapterN.hish it will populate html/&lt;storyname&gt;/ with html versions of each chapter, and keep them up to date as the markup changes. Meant to be run as a post-update hook in .git/hooks.

TODO:
* generate feeds of latest updated stories, latest updated chapters, latest created stories, etc
