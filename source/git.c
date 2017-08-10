#include "git.h"
#include "repo.h"

#include <git2/tree.h>
#include <git2/commit.h>
#include <git2/revwalk.h>
#include <git2/diff.h>

#include <string.h> // strlen, memcmp
#include <assert.h>
#include <stdio.h>

#define LITLEN(a) a,sizeof(a)-1

void git_for_commits(bool (*handle)(git_commit*)) {
	git_revwalk* walker;
	repo_check(git_revwalk_new(&walker, repo));
	// XXX: do we need to specify GIT_SORT_TIME or is that just for weird merge branch commits?
	// XXX: todo revparse HEAD~10 etc
	repo_check(git_revwalk_push_head(walker));
	git_oid oid;
	git_commit* commit;
	for(;;) {
		if(0!=git_revwalk_next(&oid, walker)) return;
		printf("rev oid %s\n",git_oid_tostr_s(&oid));
		repo_check(git_commit_lookup(&commit, repo, &oid));
		if(!handle(commit)) break;
	}
}

void git_for_chapters(chapter_handler handle) {
	git_tree* last = NULL;
	bool on_commit(git_commit* commit) {
		git_time_t timestamp = git_commit_time(commit);
		
		int file_changed(const git_diff_delta *delta,
										 float progress,
										 void *payload) {
			// can't see only directory changes and descend, have to parse >:(
			// location/markup/chapterN.hish
			const char* name = delta->new_file.path;
			size_t nlen = strlen(name);
			if(nlen < sizeof("a/markup/chapterN.hish")-1) return 0;
			const char* slash = strchr(name,'/');
			if(slash == NULL) return 0;
			const char* markup = slash+1;
			if(nlen-(markup-name) < sizeof("markup/chapterN.hish")-1) return 0;
			if(0!=memcmp(markup,LITLEN("markup/chapter"))) return 0;
			const char* num = markup + sizeof("markup/chapter")-1;
			char* end;
			long int chapnum = strtol(num,&end,10);
			if(nlen-(end-name) < sizeof(".hish")-1) return 0;
			if(0!=memcmp(end,LITLEN(".hish"))) return 0;
			// got it!

			char* location = malloc(slash-name+1);
			memcpy(location,name,slash-name);
			location[slash-name] = '\0';
			// XXX: upgrade location to a string object, so we don't have to strlen again.
			name = markup + sizeof("markup/")-1;
			// use 0-indexed chapters everywhere we can...
			if(!handle(timestamp, chapnum-1, location, name)) return -1;
		}

		git_tree* tree;
		repo_check(git_commit_tree(&tree,commit));
		if(last != NULL) {
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

			repo_check(git_diff_tree_to_tree(&diff,repo,last,tree,&opts));
			if(0 != git_diff_foreach(diff,
															 file_changed,
															 NULL, NULL, NULL)) return;
		}

		last = tree;
	}
	return git_for_commits(on_commit);
}

