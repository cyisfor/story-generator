#include <stdbool.h>
#include <git2/tree.h>
#include <git2/commit.h>

/* any of these handlers should return false to abort the perusal or true to continue.
	 don't need a "skip" option since not recursing */

/* none include a payload, because this is all fast, blocking operations. Should be able to
	 use inline functions to handle, and keep the state in their local variables, since
	 you can finish the traversal before finishing the calling function.
*/

/* handle gets passed every commit starting from HEAD and going back. */
void git_for_commits(bool (*handle)(git_commit*));

/*
	git_for_stories
	root: the directory containing your stories
	handle: passed each story
	  location: the "id" of the story, its subdirectory name
		contents: the contents of that directory, not filtered for chapters yet
*/

bool git_for_stories(git_tree* root,
										 bool (*handle)(const char* location,
																		const git_tree* contents));

/* chapter handler,
	 timestamp: time of commit (same for all chapters in a commit)
	 num: a chapter index starting from 0
	 location: the "id" of the story, its subdirectory name
	 name: the filename of the chapter chapter%d.hish
*/

typedef bool (*chapter_handler)(git_time_t timestamp,
																long int num,
																const char* location,
																const char* name);

/* git_for_chapters
	 handle: passed info on each chapter in order of modification from newest to oldest

	 this kind of puts all the above together.
*/

void git_for_chapters(chapter_handler handle);
