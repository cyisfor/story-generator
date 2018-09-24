#ifndef _STORYGIT_H_
#define _STORYGIT_H_

#include "string.h"
#include "db.h"

#include <stdbool.h>
#include <git2/tree.h>
#include <git2/commit.h>

// this is the REAL time it was committed, ignoring rebasing and such
git_time_t git_author_time(git_commit* commit);

/* any of these handlers should return false to abort the perusal or true to continue.
	 don't need a "skip" option before not recursing */

/* none include a payload, because this is all fast, blocking operations. Should be able to
	 use inline functions to handle, and keep the state in their local variables, before
	 you can finish the traversal before finishing the calling function.
*/

/* git_for_commits

	 after is where to stop, see db_last_seen_commit
	 
	 handle gets passed that commit's timestamp and tree,
	 as well as a tree of the previous commit (which will be NULL for the first commit)
	 commits are received in default order, which is ordered by time.
*/

enum gfc_action { GFC_CONTINUE = 0, GFC_STOP, GFC_SKIP };

/* SIGH
	 we need to do a special revwalk, because revwalk gimps out on diffs between merges.
	 algorithm:
	 if no parent, done
	 if single parent, diff then repeat for parent
	 if multiple parents, push left + right, then diff merge with most recent one
	 replace most recent with parent, or none
*/

struct git_item {
	git_commit* commit;
	git_tree* tree;
	git_time_t time;	
	const git_oid* oid;
};


void
git_for_commits(
	void* udata,
	enum gfc_action
	(*handle)(
		void* udata,
		const struct git_item base,
		const struct git_item parent,
		int which_parent),
	bool do_after,
	const git_time_t after,
	bool do_before,
	const db_oid before);

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
enum gfc_action
git_for_chapters_changed(
	void* udata,
	enum gfc_action
	(*handle)(
			void* udata,
			long int num,
			bool deleted,
			const string location,
			const string name),
	git_tree* parent, git_tree* base);

#endif /* _STORYGIT_H_ */
