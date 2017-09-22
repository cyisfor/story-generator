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
	bool checked;
	/* A - B - C - F - G
		  \     /
			 D - E
	  when you traverse ABCFG, then DE, don't also do another CFG
	*/
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
	struct item* todo = NULL;
	size_t ntodo = 0;

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

	/* ugh.... have to search for if a git commit has been visited or not
		 can't just say commit->visited = true, since commit is opaque
	*/
	
	for(;;) {	
		int nparents = git_commit_parentcount(me.commit);
		INFO("%.*s nparents %d\n",GIT_OID_HEXSZ,git_oid_tostr_s(me.oid),nparents);
		int i;
		for(i = 0; i < nparents; ++i) {
			struct item parent = {};
			repo_check(git_commit_parent(&parent.commit, me.commit, i));
			parent.oid = git_commit_id(parent.commit);

			if(until && git_oid_equal(GIT_OID(until), parent.oid)) continue;

			parent.time = git_commit_time(parent.commit);
			
			/* When two todo converge, the timestamp of the original common commit will be
				 earlier than any of the timestamps along either branch, so if we find the earlier
				 commits in all todo, by the time we hit the convergence, the common commit
				 will NECESSARILY be in the todo list, since it can't be processed until all
				 later commits in all todo are processed. So we just have to avoid dupes
				 in the todo list, and the nodes will all be visited exactly once.

				 Could binary search since it's sorted, but unless you're a horrible team,
				 there will only be one branch per coder, and 1-3 for development, so
				 only binary search if ntodo is large enough for the overhead of that
				 search to be lower.
			*/

			bool alreadyhere = false;
			int here;
			void findit_sametime(int j) {
				do {
					if(git_oid_equal(parent.oid, todo[j].oid)) {
						git_commit_free(parent.commit);
						// tree isn't allocated yet
						parent = todo[j];
						// WHEN I AM
						alreadyhere = true;
						here = j;
					}
					if(++j==ntodo) break;
				} while(parent.time == ntodo[j].time);
			}
			if(ntodo < 10) {
				void findit(void) {
					int j=0;
					for(;j<ntodo;++j) {
						if(parent.time == ntodo[j].time) {
							return findit_sametime(j);
						}
					}
				}
				// WHEN I AM
				findit();
			} else {
				void findit(void) {
					int j = ntodo >> 1;
					int dj = j >> 1;
					do {
						if(todo[j].time == parent.time) {
							return findit_sametime(j);
						}
						// sorted from earlier to later...
						if(todo[j].time > parent.time) {
							j -= dj;
						} else {
							j += dj;
						}
						dj >>= 1;
					} while(dj > 0);
				}
				findit();
			}

			repo_check(git_commit_tree(&parent.tree, parent.commit));
			struct action op = handle(DB_OID(*me.oid),
											 DB_OID(*parent.oid),
											 me.time,
											 parent.tree,
											 me.tree);
			switch(op) {
			case STOP:
				freeitem(parent);
				freeitem(me);
				for(i=0;i<ntodo;++i) {
					freeitem(todo[i]);
				}
				free(todo);
				return false;
			case SKIP:
				freeitem(parent);
				if(alreadyhere) {
					// ugh... need to remove this from the list
					int j;
					for(j=here;j<ntodo-1;++j) {
						todo[j] = todo[j+1];
					}
				}
				continue;
			}

			if(alreadyhere) {
				INFO("ALREADY HERE %.*s",GIT_OID_HEXSZ,git_oid_tostr_s(parent.oid));
				continue;
			}
			/* note: parents can branch, so ntodo is 3 in that case,
				 then 4 if a grandparent todo, etc */
			
			++ntodo;
			todo = realloc(todo,sizeof(*todo)*ntodo);
			//if(alreadyhere) blah blah meh
			todo[ntodo-1] = parent;
		}
		freeitem(me);
		// if we pushed all the parents, and still no todo, we're done, yay!
		if(ntodo == 0) break;

		qsort(todo, ntodo, sizeof(*todo), later_todo_last);
		// todo[ntodo] should be the most recent now.
		// pop it off, to examine its parents
		me = todo[--ntodo];
	}

	assert(0 == ntodo);
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
