#include <git2/tree.h>
#include <git2/commit.h>
#include <git2/revwalk.h>


/* any of these handlers should return false to abort the perusal or true to continue.
	 don't need a "skip" option since not recursing */

/* none include a payload, because this is all fast, blocking operations. Should be able to
	 use inline functions to handle, and keep the state in their local variables, since
	 you can finish the traversal before finishing the calling function.
*/

/* chapter handler,
	 timestamp: time of commit (same for all chapters in a commit)
	 num: a chapter index starting from 0
	 name: the filename of the chapter chapter%d.hish
*/

bool (*chapter_handler)(git_time_t timestamp,
												long int num,
												const char* location,
												const char* name);


/* handle gets passed every commit starting from HEAD and going back. */
void git_for_commits(enum action (*handle)(git_commit*)) {
	check(git_revwalk_new(&swalker, repo));
	// XXX: do we need to specify GIT_SORT_TIME or is that just for weird merge branch commits?
	// XXX: todo revparse HEAD~10 etc
	check(git_revwalk_push_head(swalker));
	git_oid oid;
	git_commit commit;
	for(;;) {
		if(0!=git_revwalk_next(&oid, walker)) return NULL;
		check(git_commit_lookup(&commit, repo, &oid));
		if(!handle(commit, payload)) break;
	}
}

/*
	git_for_stories
	root: the directory containing your stories
	handle: passed each story
	  location: the "id" of the story, its subdirectory name
		contents: the contents of that directory, not filtered for chapters yet
*/

enum action git_for_stories(git_tree* root,
								 enum action (*handle)(const char* location,
																			 const git_tree_entry* contents)) {
	size_t count = git_tree_entrycount(root);
	size_t i;
	for(i=0;i<count;++i) {
		const git_tree_entry * story = git_tree_entry_byindex(root,i);
		// stories are directories with "markup" folder in them.
		if(git_tree_entry_type(story) == GIT_OBJ_TREE) {
			const git_tree_entry* markup = git_tree_entry_byname(story, "markup");
			if(git_tree_entry_type(markup) != GIT_OBJ_TREE)
				continue;
			git_tree* contents=NULL;
			check(git_tree_lookup(&contents, repo, git_tree_entry_id(markup)));
			if(ABORT == handle(git_tree_entry_name(story),
												 contents))
				return ABORT;
		}
	}
	return OK;
}

/* git_for_chapters
	 handle: passed info on each chapter in order of modification from newest to oldest

	 this kind of puts all the above together.
*/

void git_for_chapters(chapter_handler handle) {
	enum action on_commit(git_commit* commit) {
		git_time_t timestamp = git_commit_time(commit);
		enum action on_story(const char* location,
												 const git_tree* contents) {
			size_t i=0;
			size_t count = git_tree_entrycount(contents);
			for(;i<count;++i) {
				const git_tree_entry* chapter = git_tree_entry_byindex(root,i);
				const char* name = git_tree_entry_name(chapter);
				size_t len = strlen(name);
				if(len < sizeof("chapter1.hish")) continue;
				if(0!=memcmp(name,LITLEN("chapter"))) continue;
				// position at the number...
				const char* end = name + sizeof("chapter")-1;
				// parse the number
				long int chapnum = strtol(end, &end, 10);
				// now end points to the .hish part... make sure of this
				if(len-(end-name) < sizeof(".hish")-1) continue;
				if(0!=memcmp(end,LITLEN(".hish"))) continue;
				assert(chapnum>0);
				// got it!
				// use 0-indexed chapters everywhere we can...
				handle(timestamp, chapnum-1, location, name);
			}
		}
		git_tree tree;
		check(git_commit_tree(&tree,&commit));

		return git_for_stories(tree, on_story);
	}
	return git_for_commits(on_commit);
}

