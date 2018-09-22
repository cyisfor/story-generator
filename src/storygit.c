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

git_time_t git_author_time(git_commit* commit) {
	// the commit time, or the committer time, changes with every rebase
	// but author remains the same!
	const git_signature* author = git_commit_author(commit);
	return author->when.time;
}


int later_commits_last(const void* a, const void* b) {
//	printf("um %p %p\n",a,b);
	git_time_t ta = (((struct item*) a)->time);
	git_time_t tb = (((struct item*) b)->time);
	return ta - tb;
}

void freeitem(struct item* i) {
	git_commit_free(i->commit);
	git_tree_free(i->tree);
	i->commit = NULL; // debugging
}

const char* myctime(time_t time) {
	char* ret = ctime(&time);
	ret[strlen(ret)-1] = '\0'; // stupid newline
	return ret;
}

void
git_for_commits(
	void* udata,
	enum gfc_action
	(*handle)(
		void* udata,
		const db_oid commit,
		const db_oid parent,
		git_time_t timestamp,
		git_tree* last,
		git_tree* cur),
	bool do_after,
	const git_time_t after,
	bool do_before,
	const db_oid before) {

	struct item me = {};
	struct item* todo = NULL;
	size_t ntodo = 0;

	if(do_before) {
		SPAM("before %s\n",db_oid_str(before));
		repo_check(git_commit_lookup(&me.commit, repo, GIT_OID(before)));
	} else {
		// before HEAD
		git_reference* ref;
		repo_check(git_repository_head(&ref, repo)); // this resolves the reference
		const git_oid * oid = git_reference_target(ref);
		repo_check(git_commit_lookup(&me.commit, repo, oid));
		git_reference_free(ref);
	}

	me.time = git_author_time(me.commit);
	if(do_after && me.time <= after) {
		git_commit_free(me.commit);
		return;
	}
	repo_check(git_commit_tree(&me.tree,me.commit));
	me.oid = git_commit_id(me.commit);

	/* Okay... so...

		 visit commits back until the earliest commit, that is not before "after"
		 if there are 2 parents, recurse on each parent.
		 Eventually all branches will be before "after" and you're done.
	 */

	void visit_commits(

	/* ugh.... have to search for if a git commit has been visited or not
		 can't just say commit->visited = true, before commit is opaque
	*/
	
	for(;;) {	
		int nparents = git_commit_parentcount(me.commit);
		//INFO("%.*s nparents %d\n",GIT_OID_HEXSZ,git_oid_tostr_s(me.oid),nparents);
		int i;
		for(i = 0; i < nparents; ++i) {
			struct item parent = {};
			repo_check(git_commit_parent(&parent.commit, me.commit, i));

			parent.time = git_author_time(parent.commit);
			if(do_after && parent.time < after) {
				git_commit_free(parent.commit);
				continue;
			}
			parent.oid = git_commit_id(parent.commit);

			/* When two branches converge, the timestamp of the original common commit will be
				 earlier than any of the timestamps along either branch, so if we find the earlier
				 commits in all branches, by the time we hit the convergence, the common commit
				 will NECESSARILY be in the todo list, before it can't be processed after all
				 later commits in all branches are processed. So we just have to avoid dupes
				 in the todo list, and the nodes will all be visited exactly once.

				 so with
				  A - B - C - F - G
					\     /
					 D - E

				 first visit A, then the todo list is (B,D)
				 then visit B, the todo list is (D,C) D MUST have a later timestamp than C.
				 then visit D, the todo list is (E,C) E MUST have a later timestamp than C.
				 so we always visit the node with the latest timestamp
				 visit E, and the next commit is C, but we eliminate duplicates in the todo, so (C,C) -> (C)
				 visit C, go to F
				 visit F, go to G
				 all nodes have been visited once.

				 Take a look at example_commit_tree.dot and here's how the algorithm would work
				 First, A goes in the todo.
				 Every iteration, we take the node with the highest timestamp, visit it, and add its parents to the todo, unless they're already in it.
				 So visit A, which has timestamp 10 incidentally, and add nodes H, B and D.
				 Now of (H=9,B=6,D=8) H has the highest timestamp.
				 So visit H, adding I: (I=8,B=6,D=8)
				 Visit either I or D, let's say we visit I. Push its parents to get:
				 (F=4, J=7, B=6, D=8)
				 Now D has the highest timestamp. Visit, add parents.
				 (F=4, J=7, B=6, E=6)
				 Now J is the highest, visit it, but its parent (B) is already in the todo! So we can't add it twice.
				 (F=4, B=6, E=6)
				 Now either B or E, let's visit E.
				 (F=4, B=6, C=5)
				 Now visit B, and its parent is C. Oops, already in the todo!
				 (F=4, C=5)
				 Now visit C, its parent is F
				 (F=4)
				 Now visit F
				 (G=3)
				 Now visit G

				 Now visited in timestamp order from soonest to earliest is AHIDJEBCFG or in alpha
				 

				 Could binary search before it's sorted, but unless you're a horrible team,
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
				} while(parent.time == todo[j].time);
			}
			if(ntodo < 10) {
				void findit(void) {
					int j=0;
					for(;j<ntodo;++j) {
						if(parent.time == todo[j].time) {
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
			{
				INFO("%s:%d ->",git_oid_tostr_s(parent.oid), parent.time);
				fprintf(stderr,"     %s:%d\n",git_oid_tostr_s(me.oid), me.time);
			}

			if(nparents == 1) {
				enum gfc_action op = handle(udata,
																		DB_OID(*me.oid),
																		DB_OID(*parent.oid),
																		me.time,
																		me.tree,
																		parent.tree);
				switch(op) {
				case GFC_STOP:
					freeitem(&parent);
					freeitem(&me);
					for(i=0;i<ntodo;++i) {
						freeitem(&todo[i]);
					}
					free(todo);
					return;
				case GFC_SKIP:
					freeitem(&parent);
					if(alreadyhere) {
						// ugh... need to remove this from the list
						int j;
						for(j=here;j<ntodo-1;++j) {
							todo[j] = todo[j+1];
						}
					}
					continue;
				};
				// default:

			} else {
				INFO("skipping merge commit %.*s",GIT_OID_HEXSZ,
						 git_oid_tostr_s(me.oid));
			}

			if(alreadyhere) {
				//INFO("ALREADY HERE %.*s",GIT_OID_HEXSZ,git_oid_tostr_s(parent.oid));
				continue;
			}

			/* note: parents can branch, so ntodo is 3 in that case,
				 then 4 if a grandparent todo, etc */
			
			++ntodo;
			todo = realloc(todo,sizeof(*todo)*ntodo);
			//if(alreadyhere) blah blah meh
			todo[ntodo-1] = parent;
		}
		freeitem(&me);
		// if we pushed all the parents, and still no todo, we're done, yay!
		if(ntodo == 0) break;

		qsort(todo, ntodo, sizeof(*todo), later_commits_last);
		// todo[ntodo] should be the most recent now.
		// pop it off, to examine its parents
		me = todo[--ntodo];
	}

	assert(0 == ntodo);
	assert(me.commit == NULL);
}

// note: this is the DIFF not the changes of each commit in between.
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
	git_tree* from, git_tree* to) {
	if(from == NULL) return true;
	assert(to != NULL);

	enum gfc_action result = GFC_CONTINUE;

	int file_changed(const git_diff_delta *delta,
									 float progress,
									 void *payload) {
		// can't see only directory changes and descend, have to parse >:(
		/* location/markup/chapterN.hish */
		
		/* either deleted, pass old_file, true
			 renamed, old_file, true, new_file, false
			 otherwise, new_file, false */
		
		void
			one_file(const char* spath, bool deleted)
		{
			const string path = {
				.s = spath,
				.l = strlen(spath)
			};
			if(path.l < LITSIZ("a/markup/chapterN.hish")) return;
			const char* slash = strchr(path.s,'/');
			if(slash == NULL) return;
			const char* markup = slash+1;
			if(path.l-(markup-path.s) < LITSIZ("markup/chapterN.hish")) return;
			if(0!=memcmp(markup,LITLEN("markup/chapter"))) return;
			const char* num = markup + LITSIZ("markup/chapter");
			char* end;
			long int chapnum = strtol(num,&end,10);
			assert(chapnum != 0);
			if(path.l-(end-path.s) < LITSIZ(".hish")) return;
			if(0!=memcmp(end,LITLEN(".hish"))) return;
			// got it!

			const string location = {
				.s = path.s,
				.l = slash-path.s
			};

			result = handle(udata, chapnum, deleted, location, path);
		}
		// XXX: todo: handle if unreadable
		switch(delta->status) {
		case GIT_DELTA_DELETED:
			one_file(delta->old_file.path,true);
			return result;
		case GIT_DELTA_RENAMED:
			one_file(delta->old_file.path,true);
			if(result!=GFC_CONTINUE) return result;
			// fall through
		case GIT_DELTA_ADDED:
		case GIT_DELTA_MODIFIED:
		case GIT_DELTA_COPIED:
			// note: with copied, the old file didn't change, so disregard it.
			one_file(delta->new_file.path,false);
			return result;
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
	git_diff_foreach(diff,
									 file_changed,
									 NULL, NULL, NULL, NULL);
	return result;
}
