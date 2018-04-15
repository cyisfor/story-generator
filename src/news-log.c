#include "storygit.h"

int main(int argc, char *argv[])
{
	repo_check(repo_discover_init(LITLEN(".git")));

	git_revwalk* walk;
	repo_check(git_revwalk_new(&walk, repo));
	git_revwalk_sorting(GIT_SORT_TIME);
	
	// since HEAD
	git_reference* ref;
	git_commit* commit;
	repo_check(git_repository_head(&ref, repo)); // this resolves the reference
	const git_oid * oid = git_reference_target(ref);
	git_reference_free(ref);

	git_revwalk_push(walk, oid);

	
	return 0;
}
