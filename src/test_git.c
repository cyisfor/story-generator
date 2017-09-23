#include "repo.h"
#include "storygit.h"
#include <stdio.h>

int main(int argc, char *argv[])
{

	enum gfc_action on_commit(
		const db_oid commit,
		const db_oid parent,
		git_time_t timestamp,
		git_tree* last,
		git_tree* cur)
	{

		printf("timestamp %d\n",timestamp);
		
		enum gfc_action on_chapter(
			long int num,
			bool deleted,
			const string location,
			const string path)
		{
			printf("%2d ",num);
			int i;
			for(i=0;i<20-location.l;++i) putchar(' ');
			STRPRINT(location);
			putchar(' ');
			STRPRINT(path);
			putchar('\n');
			return GFC_CONTINUE;
		}
		return git_for_chapters_changed(last,cur,on_chapter);
	}

	puts("chapter location name");

	repo_discover_init(".",1);
	git_for_commits(false, NULL, false, 0, on_commit);

	return 0;
}
