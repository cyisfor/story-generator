#include "storydb.h"

void each_chapter(
		void* udata,
		identifier story,
		const string location,
		identifier chapter,
		git_time_t timestamp) {
	printf("%d:%.*s %d %d\n",story,(int)location.len, location.base, chapter,timestamp);
}

int main(int argc, char *argv[])
{
	storydb_open();
	storydb_for_unpublished_chapters(NULL, 100, each_chapter);
	return 0;
}
