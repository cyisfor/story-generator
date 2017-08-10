#include "repo.h"
#include "git.h"

int main(int argc, char *argv[])
{
	bool on_chapter(git_time_t timestamp,
									long int num,
									const char* location,
									const char* name) {
		printf("%d %d %s %s\n",timestamp,num,location,name);
	}

	repo_discover_init(".",1);
	git_for_chapters(on_chapter);

	return 0;
}
