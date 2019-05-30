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

git_time_t git_author_time(git_commit* commit) {
	// the commit time, or the committer time, changes with every rebase
	// but author remains the same!
	const git_signature* author = git_commit_author(commit);
	return author->when.time;
}


int later_commits_last(const void* a, const void* b) {
//	printf("um %p %p\n",a,b);
	git_time_t ta = (((struct git_item*) a)->time);
	git_time_t tb = (((struct git_item*) b)->time);
	return ta - tb;
}

static
void freeitem(struct git_item* i) {
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
		const struct git_item base,
		const struct git_item parent,
		int which_parent),
	bool do_after,
	const git_time_t after,
	bool do_before,
	const db_oid before) {

	struct git_item me = {};
	struct git_item* todo = NULL;
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

	/* ugh.... have to search for if a git commit has been visited or not
		 can't just say commit->visited = true, before commit is opaque
	*/
	
	for(;;) {	
		int nparents = git_commit_parentcount(me.commit);
		//INFO("%.*s nparents %d\n",GIT_OID_HEXSZ,git_oid_tostr_s(me.oid),nparents);
		int i;
		for(i = 0; i < nparents; ++i) {
			struct git_item parent = {};
			repo_check(git_commit_parent(&parent.commit, me.commit, i));

			parent.time = git_author_time(parent.commit);
			if(do_after && parent.time < after) {
				git_commit_free(parent.commit);
				continue;
			}
			parent.oid = git_commit_id(parent.commit);

			// PUSH CURRENT PARENT INTO TODO LIST

			/* When two branches converge, the timestamp of the original common
				 commit will be earlier than any of the timestamps along either branch,
				 so if we find the earlier commits in all branches, by the time we hit
				 the convergence, the common commit will NECESSARILY be in the todo
				 list, before it can't be processed after all later commits in all
				 branches are processed. So we just have to avoid dupes in the todo
				 list, and the nodes will all be visited exactly once.

				 so with
				  A - B - C - F - G
					\     /
					 D - E

				 first visit A, then the todo list is (B,D)
				 then visit B, the todo list is (D,C)
				 D MUST have a later timestamp than C.
				 then visit D, the todo list is (E,C)
				 E MUST have a later timestamp than C.
				 so we always visit the node with the latest timestamp
				 visit E, and the next commit is C, but we eliminate duplicates in the
				 todo, so (C,C) -> (C)
				 visit C, go to F
				 visit F, go to G
				 all nodes have been visited once.

				 Take a look at example_commit_tree.dot and here's how the algorithm
				 would work.
				 First, A goes in the todo.
				 (A=10)
				 Every iteration, we take the node with the highest timestamp,
				 visit it, and add its parents to the todo, unless they're already
				 in it.

				 So visit A, which has timestamp 10 incidentally,
				 and add nodes B, F,  and D
				 (B=9,F=6,D=8)
				 Now, of the three, B has the highest timestamp.
				 So visit B, adding C
				 (C=8,F=6,D=8)
				 Visit either C or D, let's say we visit C. Push its parents to get:
				 (I=4,E=7,F=6,D=8)
				 Now D is the highest, so visit it, and push G
				 (I=4,E=7,F=6,G=6)
				 Now E is the highest, so visit it, and push F. But wait!
				 F is already in the todo list! So just leave it as
				 (I=4,F=6,G=6)
				 We can visit either F or G now, so let's do G.
				 (I=4,F=6,H=5)
				 Now visit F, discarding its parent, since H is already in the list
				 (I=4,H=5)
				 Visit H, and its parent is I, so the todo list is now
				 (I=4)
				 Visit I.
				 (J=3)
				 Visit J.
				
				 Now visited in timestamp order from soonest to earliest is ABCDEFGHIJ
				 All nodes have been visited!

				 This is a good idea, because normally with a DAG, you have to keep
				 a list of ALL visited nodes. But this DAG has timestamp values that
				 follow a special order, such that everything upstream, even in
				 neighboring branches, was created earlier than everything downstream.
				 So we only need to keep a list of "todo" nodes, and use the timestamp
				 to ensure we visit everything.

				 Unless you're a horrible team,
				 there will only be one branch per coder, and 1-3 for development, so
				 only binary search if ntodo is large enough for the overhead of that
				 search to be lower. Otherwise just iterate through it, looking for
				 the max timestamp. If ntodo is large enough, sort the todo
				 list with every addition, then use binary search until it's smaller.

				 Really should figure out some sort of heap thing here, eh. I no good
				 at computer.
			*/

			// we're searching the todo list for the current parent.
			// first find matching timestamps
			// then compare longer OIDs

			/* The todo list is always sorted by commit time.
				 So if you run into the correct commit time, all possible OIDs
				 are going to be next to each other
			 */
			
			bool alreadyhere = false;
			int here;
			void check_one_timestamp(int j) {
				do {
					if(git_oid_equal(parent.oid, todo[j].oid)) {
						// okay, so we have parent.commit and todo[j].commit both
						// allocated. Let's replace parent.commit
						git_commit_free(parent.commit);
						parent.commit = todo[j].commit;
						// parent.tree isn't allocated yet
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
							// it's gotta be between j and ?
							return check_one_timestamp(j);
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
							// it's gotta be between j and ?
							return check_one_timestamp(j);
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

			if(!alreadyhere) {
				++ntodo;
				todo = realloc(todo,sizeof(*todo)*ntodo);
				todo[ntodo-1] = parent;
				qsort(todo, ntodo, sizeof(*todo), later_commits_last);
			}

			// if a parent's already here, still visit it, because
			// a different branch may have different changes to it
			
			// VISIT CURRENT NODE W/ CURRENT PARENT
			
			// we need the tree of the parent, just for visiting purposes
			repo_check(git_commit_tree(&parent.tree, parent.commit));
			{
				INFO("%s:%d ->",git_oid_tostr_s(parent.oid), parent.time);
				fprintf(stderr,"     %s:%ld\n",git_oid_tostr_s(me.oid), me.time);
			}

			/* actually, have to visit once per parent, for chapters changed diff
				 between the parents of merge commits.
			*/
			enum gfc_action op = handle(udata,me,parent,i);
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
				// don't add the parent to our todo list.
				freeitem(&parent);
				if(alreadyhere) {
					// ugh... maybe need to remove this from the list, if already in it
					// from a previous branch
					int j;
					for(j=here;j<ntodo-1;++j) {
						todo[j] = todo[j+1];
					}
				}
				continue;
			};
		}
		freeitem(&me);
		// if we pushed all the parents, and still no todo, we're done, yay!
		if(ntodo == 0) break;

		// since LATER are LAST,
		// todo[ntodo] should be the most recent now.
		// pop it off, to visit it and examine its parents
		me = todo[--ntodo];
		// don't bother reallocing.
	}

	assert(0 == ntodo);
	assert(me.commit == NULL);
}

typedef enum gfc_action
	(*chapter_handler)(
			void* udata,
			long int num,
			bool deleted,
			const string location,
			const string name);

struct diffctx {
	chapter_handler handle;
	void* udata;
	enum gfc_action result;
};

static
int file_changed(const git_diff_delta *delta,
								 float progress,
								 struct diffctx *ctx) {

	// can't see only directory changes and descend, have to parse >:(
	/* location/markup/chapterN.hish */
		
	/* either deleted, pass old_file, true
		 renamed, old_file, true, new_file, false
		 otherwise, new_file, false */

	void
		one_file(const char* spath, bool deleted)
	{
		const string path = {
			.base = spath,
			.len = strlen(spath)
		};
		if(path.len < LITSIZ("a/markup/chapterN.hish")) return;
		const char* slash = strchr(path.base,'/');
		if(slash == NULL) return;
		const char* markup = slash+1;
		if(path.len-(markup-path.base) < LITSIZ("markup/chapterN.hish")) return;
		if(0!=memcmp(markup,LITLEN("markup/chapter"))) return;
		const char* num = markup + LITSIZ("markup/chapter");
		char* end;
		long int chapnum = strtol(num,&end,10);
		assert(chapnum != 0);
		if(path.len-(end-path.base) < LITSIZ(".hish")) return;
		if(0!=memcmp(end,LITLEN(".hish"))) return;
		// got it!

		const string location = {
			.base = path.base,
			.len = slash-path.base
		};

		ctx->result = ctx->handle(ctx->udata, chapnum, deleted, location, path);
	}
	// XXX: todo: handle if unreadable
	switch(delta->status) {
	case GIT_DELTA_DELETED:
		one_file(delta->old_file.path,true);
		return ctx->result;
	case GIT_DELTA_RENAMED:
		one_file(delta->old_file.path,true);
		if(ctx->result!=GFC_CONTINUE) return ctx->result;
		// fall through
	case GIT_DELTA_ADDED:
	case GIT_DELTA_MODIFIED:
	case GIT_DELTA_COPIED:
		// note: with copied, the old file didn't change, so disregard it.
		one_file(delta->new_file.path,false);
		return ctx->result;
	default:
		ERROR("bad delta status %d",delta->status);
		abort();
	};
}


// note: this is the DIFF not the changes of each commit in between.
enum gfc_action
git_for_chapters_changed(
	void* udata,
	chapter_handler handler,
	git_tree* parent, git_tree* base) {
	if(base == NULL) return true;
	assert(parent != NULL);

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

	repo_check(git_diff_tree_to_tree(&diff,repo,base,parent,&opts));
	struct diffctx ctx = {
		.handle = handler,
		.udata = udata,
		.result = GFC_CONTINUE
	};
	git_diff_foreach(diff,
									 (void*)file_changed,
									 NULL, NULL, NULL, &ctx);
	return ctx.result;
}
