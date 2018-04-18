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

void output_story(identifier story,
									const string location,
									size_t ready,
									size_t numchaps,
									git_time_t timestamp) {
	string latest, bleeding_edge;
	CHAPTER_NAME_STRING(numchaps,
											bleeding_edge,
											derpbuf1);
	ready = ready ? ready : numchaps > 1 ? numchaps -1 : 1
	CHAPTER_NAME_STRING(ready,
											latest,
											derpbuf2);
	latest.which = ready;
	char modbuf[0x100];
	char modbuf2[0x100]; // sigh
	string modified = {
		modbuf,
		strftime(modbuf, 0x100, "%B %e, %Y", localtime(&timestamp))
	};
	string modified_time = {
		modbuf2,
		strftime(modbuf2, 0x100, "%I:%M %p", localtime(&timestamp))
	};
	void derp(string title, string description, string source) {
		if(!title.l) {
			title = location;
		}
#include "o/template/contents-story.html.c"
	}
	storydb_with_info(story, derp);
}

int main(int argc, char *argv[])
{
	db_open();
	void output_stories() {
		storydb_for_stories(output_story, false, 0);
	}
	string title = {
		.s = "Table of Contents",
		.l = LITSIZ("Table of Contents")
	};
	void output_body() {
#include "o/template/contents-body.html.c"
	}
#include "o/template/page.html.c"
	return 0;
}
