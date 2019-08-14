#include "storydb.h"
#include "mystring.h"
#include "create.h"

#include <stdio.h>

#define output_buf(s,l) fwrite(s,l,1,stdout)
#define output_literal(lit) output_buf(lit,sizeof(lit)-1)
#define output_fmt printf

struct chapter {
	string name;
	size_t which;
};

struct csucks {
	string location;
	string modified;
	string modified_time;
	string latest;
	size_t ready;
	size_t numchaps;
	string bleeding_edge;
	identifier story;
};

static
void have_info(void* udata, string title, string description, string source) {
	struct csucks* g = (struct csucks*) udata;
	if(!title.len) {
		title = g->location;
	}
	bool under_construction = storydb_under_construction(
	const string location = g->location;
	const string modified = g->modified;
	const string modified_time = g->modified_time;
	const string latest = g->latest;
	size_t ready = g->ready;
	size_t numchaps = g->numchaps;
	const string bleeding_edge = g->bleeding_edge;

#include "template/contents-story.html.c"
}

#define output_buf(s,l) fwrite(s,l,1,stdout)
#define output_literal(lit) output_buf(lit,sizeof(lit)-1)
#define output_fmt printf

void output_story(void* udata,
									identifier story,
									const string location,
									size_t ready,
									size_t numchaps,
									git_time_t timestamp) {
	struct csucks g = {
		.location = location,
		.story = story,
		.ready = ready,
		.numchaps = numchaps
	};
	char derpbuf1[0x100], derpbuf2[0x100];
	if(ready != numchaps) {
		CHAPTER_NAME_STRING(ready + 1,
												g.bleeding_edge,
												derpbuf1);
	}
	CHAPTER_NAME_STRING(ready,
											g.latest,
											derpbuf2);
	char modbuf[0x100];
	char modbuf2[0x100]; // sigh
	g.modified.base = modbuf;
	g.modified.len = strftime(modbuf, 0x100, "%B %e, %Y", localtime(&timestamp));
	g.modified_time.base = modbuf2;
	g.modified_time.len = strftime(modbuf2, 0x100,
															 "%I:%M %p", localtime(&timestamp));

	storydb_with_info(&g, have_info, story);
}

int main(int argc, char *argv[])
{
	storydb_open();
	void output_stories() {
		storydb_for_stories(NULL, output_story, false, 0);
	}
	string title = {
		.base = "Table of Contents",
		.len = LITSIZ("Table of Contents")
	};
	void output_body() {
#include "template/contents-body.html.c"
	}
#include "template/page.html.c"
	return 0;
}
