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

bool git_for_commits(const db_oid until,
										 const db_oid since, 
										 bool (*handle)(db_oid commit,
																		git_time_t timestamp,
																		git_tree* last,
																		git_tree* cur)) {
	git_revwalk* walker=NULL;
	repo_check(git_revwalk_new(&walker, repo));

	// XXX: do we need to specify GIT_SORT_TIME or is that just for weird merge branch commits?
	git_revwalk_sorting(walker, GIT_SORT_TIME);
	
	// XXX: todo revparse HEAD~10 etc
	if(since) {
		SPAM("since %s\n",db_oid_str(since));
		repo_check(git_revwalk_push(walker,GIT_OID(since)));
	} else {
		repo_check(git_revwalk_push_head(walker));
	}

	git_time_t last_timestamp = 0;
	if(until) {
		// IMPORTANT: be sure to go 1 past this in the commit log
		// since changes are from old->new, so it needs a starting old state.
		/* XXX: this is overcomplicated... */
		git_revwalk* derper=NULL;
		repo_check(git_revwalk_new(&derper, repo));
		git_oid derp = *(GIT_OID(until));
		repo_check(git_revwalk_push(derper,&derp));
		git_oid test;
		// first one is the one we pushed, so 2 to go back 1
		if(0==git_revwalk_next(&test, derper) &&
			 0==git_revwalk_next(&test, derper)) {
			git_commit* commit = NULL;
			// an older one exists, yay.
			repo_check(git_revwalk_hide(walker,&test));
			// get the timestamp, to stop if we miss the commit
			repo_check(git_commit_lookup(&commit, repo, &test));
			last_timestamp = git_commit_time(commit);
		} else {
			// no older version, we're going back to the beginning.
		}
		git_revwalk_free(derper);
	}
	
	git_commit* commit = NULL;
	git_tree* last = NULL;
	git_tree* cur = NULL;
	git_oid commit_oid;
	bool op() {
		git_time_t timestamp = 0;
		for(;;) {
			if(0!=git_revwalk_next(&commit_oid, walker)) {
				if(last) git_tree_free(last);
				return true;
			}
			//SPAM("rev oid %s",git_oid_tostr_s(&commit_oid));
			repo_check(git_commit_lookup(&commit, repo, &commit_oid));
			if(1!=git_commit_parentcount(commit)) {
				git_commit_free(commit);
				if(last) git_tree_free(last);
				return true;
			}
			repo_check(git_commit_tree(&cur,commit));
			if(timestamp == 0) {
				// eh, duplicate one timestamp I guess
				timestamp = git_commit_time(commit);
			}
			if(!handle(DB_OID(commit_oid),timestamp, last, cur)) return false;
			git_tree_free(last);
			last = cur;
			// timestamp should be for the NEWER tree
			timestamp = git_commit_time(commit);
			if(timestamp < last_timestamp) {
				// we missed the last commit we saw!
				git_commit_free(commit);
				if(last) git_tree_free(last);
				return true;
			}
			git_commit_free(commit);
		}
	}
	bool ret = op();
	git_revwalk_free(walker);
	return ret;
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
