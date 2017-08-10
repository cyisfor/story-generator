#include "git.h"
#include "db.h"
#include "repo.h"

#include <git2/tree.h>
#include <git2/commit.h>
#include <git2/revwalk.h>
#include <git2/diff.h>

#include <string.h> // strlen, memcmp
#include <assert.h>
#include <stdio.h>

#define LITSIZ(a) (sizeof(a)-1)
#define LITLEN(a) a,LITSIZ(a)


bool git_for_commits(const git_oid* until = NULL, // db_last_seen_commit
										 bool (*handle)(db_oid, git_time_t timestamp, git_tree* last, git_tree* cur)) {
	git_revwalk* walker=NULL;
	repo_check(git_revwalk_new(&walker, repo));

	// XXX: do we need to specify GIT_SORT_TIME or is that just for weird merge branch commits?
	// XXX: todo revparse HEAD~10 etc
	repo_check(git_revwalk_push_head(walker));

	if(until) {
		repo_check(git_revwalk_hide(walker,&until));
	}
	
	git_commit* commit = NULL;
	git_tree* last = NULL;
	git_tree* cur = NULL;
	db_oid commit_oid;
	for(;;) {
		if(0!=git_revwalk_next(&last_commit, walker)) return true;
		//printf("rev oid %s\n",git_oid_tostr_s(&oid));
		repo_check(git_commit_lookup(&commit, repo, &commit_oid));
		git_time_t timestamp = git_commit_time(commit);
		repo_check(git_commit_tree(&cur,commit));
		if(!handle(DB_OID(commit_oid),timestamp, last, cur)) return false;
		last = cur;
		// make this optional so we can parse w/o eliminating commits
		//db_saw_commit(timestamp, DB_OID(last_commit));
	}
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
		bool one_file(const char* path, bool deleted) {
			const string path = {
				.s = delta->new_file.path,
				.l = strlen(delta->new_file.path)
			};
			if(path.l < LITSIZ("a/markup/chapterN.hish")) return 0;
			const char* slash = strchr(path.s,'/');
			if(slash == NULL) return 0;
			const char* markup = slash+1;
			if(nlen-(markup-path.s) < LITSIZ("markup/chapterN.hish")) return 0;
			if(0!=memcmp(markup,LITLEN("markup/chapter"))) return 0;
			const char* num = markup + LITSIZ("markup/chapter");
			char* end;
			long int chapnum = strtol(num,&end,10);
			if(path.l-(end-path.s) < LITSIZ(".hish")) return 0;
			if(0!=memcmp(end,LITLEN(".hish"))) return 0;
			// got it!

			const string location = {
				.s = path.s,
				.l = slash-path.s
			};

			return handle(timestamp, chapnum, location, path);
		}
		// XXX: todo: handle if unreadable
		switch(delta->status) {
		case GIT_DELTA_DELETED:
			return one_file(delta->old_file,true);
		case GIT_DELTA_RENAMED:
			{ bool a = one_file(delta->old_file,true);
				if(!a) return false;
			}
			// fall through
		case GIT_DELTA_ADDED:
		case GIT_DELTA_MODIFIED:
		case GIT_DELTA_COPIED:
			// note: with copied, the old file didn't change, so disregard it.
			return one_file(delta->new_file,false);
		default:
			error(23,23,"bad delta status %d",delta->status);
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
