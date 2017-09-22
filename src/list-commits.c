#include "db_oid/base.h"
#include "db_oid/gen.h"
#include "ensure.h"
#include "repo.h"
#include "storygit.h"
#include <unistd.h> // chdir

#include <stdio.h>
#include <sys/stat.h>


int main(int argc, char *argv[])
{
	struct stat info;
	while(0 != stat("code",&info)) ensure0(chdir(".."));
	repo_check(repo_discover_init(LITLEN(".git")));

	bool on_commit(const db_oid commit,
								 const db_oid parent,
								 git_time_t timestamp,
								 git_tree* last,
								 git_tree* cur) {
		fputs(db_oid_str(parent),stdout);
		fputs(" -> ",stdout
		fputs(db_oid_str(commit),stdout);
	}
	
	git_for_commits(NULL,NULL,on_commit);
	return 0;
}
