#include "repo.h"
#include "git.h"
#include <stdio.h>

int main(int argc, char *argv[])
{
	bool on_chapter(git_time_t timestamp,
									long int num,
									string location,
									string name) {
		printf("%d %2d ",timestamp,num);
		int i;
		for(i=0;i<20-location.l;++i) putchar(' ');
		STRPRINT(location);
		putchar(' ');
		STRPRINT(name);
		putchar('\n');
		return true;
	}

	puts("timestamp chapter location name");

	repo_discover_init(".",1);
	git_for_chapters(on_chapter);

	return 0;
}
