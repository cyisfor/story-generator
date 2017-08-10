#include "string.h"

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

/* chapter handler,
	 timestamp: time of commit (same for all chapters in a commit)
	 num: a chapter index starting from 0
	 location: the "id" of the story, its subdirectory name
	 name: the filename of the chapter chapter%d.hish

	 note: location/name are no good outside the handler
*/

typedef bool (*chapter_handler)(git_time_t timestamp,
																long int num,
																const string location,
																const string name);

/* git_for_chapters
	 handle: passed info on each chapter in order of modification from newest to oldest

	 the same timestamp for a given commit, it doesn't really distinguish between commits,
	 treating it like a sequence of file changes. chapters can and will repeat!
*/

void git_for_chapters(chapter_handler handle);
