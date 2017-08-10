#include "repo.h"
#include "storygit.h"
#include <stdio.h>

int main(int argc, char *argv[])
{

	bool on_commit(db_oid commit,
								 git_time_t timestamp,
								 git_tree* last,
								 git_tree* cur) {

		printf("timestamp %d\n",timestamp);
		
s		bool on_chapter(long int num,
										bool deleted,
										const string location,
										const string path) {
			printf("%2d ",num);
			int i;
			for(i=0;i<20-location.l;++i) putchar(' ');
			STRPRINT(location);
			putchar(' ');
			STRPRINT(path);
			putchar('\n');
			return true;
		}
		git_for_chapters_changed(last,cur,on_chapter);
	}

	puts("chapter location name");

	repo_discover_init(".",1);
	git_for_commits(on_chapter);

	return 0;
}
