#include "git.h"
#include "repo.h"

#include <git2/tree.h>
#include <git2/commit.h>
#include <git2/revwalk.h>

#include <string.h> // strlen, memcmp
#include <assert.h>

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

bool git_for_stories(git_tree* root,
										 bool (*handle)(const char* location,
																		const git_tree* contents)) {
	size_t count = git_tree_entrycount(root);
	size_t i;
	for(i=0;i<count;++i) {
		// stories are directories with "markup" folder in them.
		const git_tree_entry * storyent = git_tree_entry_byindex(root,i);
		if(git_tree_entry_type(storyent) != GIT_OBJ_TREE) continue;
		git_tree* story=NULL;
		repo_check(git_tree_lookup(&story, repo, git_tree_entry_id(storyent)));
		const git_tree_entry* markup = git_tree_entry_byname(story, "markup");
		if(markup == NULL) continue;
		if(git_tree_entry_type(markup) != GIT_OBJ_TREE) continue;
		git_tree* contents=NULL;
		repo_check(git_tree_lookup(&contents, repo, git_tree_entry_id(markup)));
		if(!handle(git_tree_entry_name(storyent), contents)) return false;
	}
	return true;
}

void git_for_chapters(chapter_handler handle) {
	bool on_commit(git_commit* commit) {
		git_time_t timestamp = git_commit_time(commit);
		bool on_story(const char* location,
												 const git_tree* contents) {
			size_t i=0;
			size_t count = git_tree_entrycount(contents);
			for(;i<count;++i) {
				const git_tree_entry* chapter = git_tree_entry_byindex(contents,i);
				const char* name = git_tree_entry_name(chapter);
				size_t len = strlen(name);
				if(len < sizeof("chapter1.hish")-1) continue;
				if(0!=memcmp(name,LITLEN("chapter"))) continue;
				// position at the number...
				char* end = (char*)name + sizeof("chapter")-1;
				// parse the number
				long int chapnum = strtol(end, &end, 10);
				// now end points to the .hish part... make sure of this
				if(len-(end-name) < sizeof(".hish")-1) continue;
				if(0!=memcmp(end,LITLEN(".hish"))) continue;
				assert(chapnum>0);
				// got it!
				// use 0-indexed chapters everywhere we can...
				return handle(timestamp, chapnum-1, location, name);
			}
			return true;
		}
		git_tree* tree;
		repo_check(git_commit_tree(&tree,commit));

		return git_for_stories(tree, on_story);
	}
	return git_for_commits(on_commit);
}

