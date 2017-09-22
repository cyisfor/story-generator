#include "repo.h"
#include "storygit.h"

int main(int argc, char *argv[])
{
	struct stat info;
	while(0 != stat("code",&info)) ensure0(chdir(".."));
	repo_check(repo_discover_init(LITLEN(".git")));

	bool on_commit(db_oid commit,
								 git_time_t timestamp,
								 git_tree* last,
								 git_tree* cur) {
		puts(db_oid_str(commit));
	}
	
	git_for_commits(NULL,NULL,on_commit);
	return 0;
}
