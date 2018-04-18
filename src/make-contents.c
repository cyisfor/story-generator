#include "db.h"
#include "mystring.h"
#include "create.h"

#include <stdio.h>

#define output_buf(s,l) fwrite(s,l,1,stdout)
#define output_literal(lit) output_buf(lit,sizeof(lit)-1)
#define output_fmt printf

void output_story(identifier story,
									const string location,
									bool finished,
									size_t numchaps,
									git_time_t timestamp) {
	if(!finished) --numchaps;
	CHAPTER_NAME(numchaps + 1, bleeding_edge);
	CHAPTER_NAME(numchaps, latest);
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
	db_with_story_info(story, derp);
}

int main(int argc, char *argv[])
{
	db_open();
	void output_stories() {
		db_for_stories(output_story, false, 0);
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
