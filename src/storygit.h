#ifndef _STORYGIT_H_
#define _STORYGIT_H_

#include "string.h"
#include "db.h"

#include <stdbool.h>
#include <git2/tree.h>
#include <git2/commit.h>

/* any of these handlers should return false to abort the perusal or true to continue.
	 don't need a "skip" option since not recursing */

/* none include a payload, because this is all fast, blocking operations. Should be able to
	 use inline functions to handle, and keep the state in their local variables, since
	 you can finish the traversal before finishing the calling function.
*/

/* git_for_commits

	 until is where to stop, see db_last_seen_commit
	 
	 handle gets passed that commit's timestamp and tree,
	 as well as a tree of the previous commit (which will be NULL for the first commit)
	 commits are received in default order, which is ordered by time.
*/

enum gfc_action { GFC_CONTINUE, GFC_STOP, GFC_SKIP };

bool git_for_commits(const db_oid until,
										 const db_oid since, 
										 enum gfc_action (*handle)(const db_oid commit,
																		const db_oid parent,
																		git_time_t timestamp,
																		git_tree* last,
																		git_tree* cur));

/*
	git_for_chapters_changed

	from can be NULL, which is a no-op
	
	chapters are defined as things matching LOCATION/markup/chapterN.hish
	indexed from 1 because chapter0.hish looks dumb
	every location passed will thus be a valid story, not just any directory.
	handle gets passed the parsed number, the (not null terminated) location, and
	the (null terminated) path found.

	note: this is the DIFF of the trees not the changes of each commit in between.
*/
bool git_for_chapters_changed(git_tree* from, git_tree* to,
															bool (*handle)(long int num,
																						 bool deleted,
																						 const string location,
																						 const string name));



#endif /* _STORYGIT_H_ */
