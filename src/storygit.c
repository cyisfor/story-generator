#include "storygit.h"
#include "db.h"
#include "repo.h"
#include "note.h"

#include <git2/tree.h>
#include <git2/commit.h>
#include <git2/revwalk.h>
#include <git2/diff.h>

#include <string.h> // strlen, memcmp
#include <assert.h>
#include <stdio.h>

#define LITSIZ(a) (sizeof(a)-1)
#define LITLEN(a) a,LITSIZ(a)

/* SIGH
	 we need to do a special revwalk, because revwalk gimps out on diffs between merges.
	 algorithm:
	 if no parent, done
	 if single parent, diff then repeat for parent
	 if multiple parents, push left + right, then diff merge with most recent one
	 replace most recent with parent, or none
*/


struct item {
	git_commit* commit;
	git_tree* tree;
	git_time_t time;	
	const git_oid* oid;
};


int later_branches_last(const void* a, const void* b) {
//	printf("um %p %p\n",a,b);
	git_time_t ta = git_commit_time(((struct item*) a)->commit);
	git_time_t tb = git_commit_time(((struct item*) b)->commit);
	return ta - tb;
}

void freeitem(struct item i) {
	git_commit_free(i.commit);
	git_tree_free(i.tree);
	i.commit = NULL; // debugging
}
	


bool git_for_commits(const db_oid until,
										 const db_oid since, 
										 bool (*handle)(const db_oid commit,
																		const db_oid parent,
																		git_time_t timestamp,
																		git_tree* last,
																		git_tree* cur)) {

	struct item me = {};
	struct item* branches = NULL;
	size_t nbranches = 0;

	if(until) {
		SPAM("until %s\n",db_oid_str(since));
		repo_check(git_commit_lookup(&me.commit, repo, GIT_OID(until)));
	} else {
		git_reference* ref;
		repo_check(git_repository_head(&ref, repo)); // this resolves the reference
		const git_oid * oid = git_reference_target(ref);
		repo_check(git_commit_lookup(&me.commit, repo, oid));
		git_reference_free(ref);
	}

	repo_check(git_commit_tree(&me.tree,me.commit));
	me.time = git_commit_time(me.commit);
	me.oid = git_commit_id(me.commit);
	
	for(;;) {	
		int nparents = git_commit_parentcount(me.commit);
		INFO("%.*s nparents %d\n",sizeof(db_oid),DB_OID(*me.oid),nparents);
		int i;
		for(i = 0; i < nparents; ++i) {
			struct item parent = {};
			repo_check(git_commit_parent(&parent.commit, me.commit, i));
			if(until && git_oid_equal(GIT_OID(until), git_commit_id(parent.commit))) continue;
			repo_check(git_commit_tree(&parent.tree, parent.commit));
			parent.oid = git_commit_id(parent.commit);
			parent.time = git_commit_time(parent.commit);
			bool ok = handle(DB_OID(*me.oid),
											 DB_OID(*parent.oid),
											 me.time,
											 parent.tree,
											 me.tree);
						 
			if(!ok) {
				freeitem(parent);
				freeitem(me);
				for(i=0;i<nbranches;++i) {
					freeitem(branches[i]);
				}
				free(branches);
				return false;
			}
			/* note: parents can branch, so nbranches is 3 in that case,
				 then 4 if a grandparent branches, etc */
			
			++nbranches;
			branches = realloc(branches,sizeof(*branches)*nbranches);
			branches[nbranches-1] = parent;
		}
		freeitem(me);
		// if we pushed all the parents, and still no branches, we're done, yay!
		if(nbranches == 0) break;

		qsort(branches, nbranches, sizeof(*branches), later_branches_last);
		// branches[nbranches] should be the most recent now.
		// pop it off, to examine its parents
		me = branches[--nbranches];
	}

	assert(0 == nbranches);
	assert(me.commit == NULL);
	return true;
}

// note: this is the DIFF not the changes of each commit in between.
bool git_for_chapters_changed(git_tree* from, git_tree* to,
															bool (*handle)(long int num,
																						 bool deleted,
																						 const string location,
																						 const string name)) {
	if(from == NULL) return true;
	assert(to != NULL);
	int file_changed(const git_diff_delta *delta,
									 float progress,
									 void *payload) {
		// can't see only directory changes and descend, have to parse >:(
		/* location/markup/chapterN.hish */
		
		/* either deleted, pass old_file, true
			 renamed, old_file, true, new_file, false
			 otherwise, new_file, false */
		bool one_file(const char* spath, bool deleted) {
			const string path = {
				.s = spath,
				.l = strlen(spath)
			};
			if(path.l < LITSIZ("a/markup/chapterN.hish")) return 0;
			const char* slash = strchr(path.s,'/');
			if(slash == NULL) return 0;
			const char* markup = slash+1;
			if(path.l-(markup-path.s) < LITSIZ("markup/chapterN.hish")) return 0;
			if(0!=memcmp(markup,LITLEN("markup/chapter"))) return 0;
			const char* num = markup + LITSIZ("markup/chapter");
			char* end;
			long int chapnum = strtol(num,&end,10);
			assert(chapnum != 0);
			if(path.l-(end-path.s) < LITSIZ(".hish")) return 0;
			if(0!=memcmp(end,LITLEN(".hish"))) return 0;
			// got it!

			const string location = {
				.s = path.s,
				.l = slash-path.s
			};

			return handle(chapnum, deleted, location, path) ? 0 : -1;
		}
		// XXX: todo: handle if unreadable
		switch(delta->status) {
		case GIT_DELTA_DELETED:
			return one_file(delta->old_file.path,true);
		case GIT_DELTA_RENAMED:
			{ bool a = one_file(delta->old_file.path,true);
				if(!a) return false;
			}
			// fall through
		case GIT_DELTA_ADDED:
		case GIT_DELTA_MODIFIED:
		case GIT_DELTA_COPIED:
			// note: with copied, the old file didn't change, so disregard it.
			return one_file(delta->new_file.path,false);
		default:
			ERROR("bad delta status %d",delta->status);
			abort();
		};
	}

	git_diff* diff=NULL;
	git_diff_options opts = {
		.version = GIT_DIFF_OPTIONS_VERSION,
		.pathspec = {NULL,0}, // all files
		.ignore_submodules = GIT_SUBMODULE_IGNORE_ALL,
		.context_lines = 0,
		.flags =
		GIT_DIFF_SKIP_BINARY_CHECK ||
		GIT_DIFF_ENABLE_FAST_UNTRACKED_DIRS
	};

	repo_check(git_diff_tree_to_tree(&diff,repo,from,to,&opts));
	if(0 == git_diff_foreach(diff,
													 file_changed,
													 NULL, NULL, NULL, NULL)) return true;
	return false;
}
