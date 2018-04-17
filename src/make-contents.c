#include "db.h"
#include "mystring.h"
#include "create.h"

#include <stdio.h>

#define output_buf(s,l) fwrite(s,l,1,stdout)
#define output_literal(lit) output_buf(lit,sizeof(lit)-1)

void output_story(identifier story,
									const string location,
									bool finished,
									size_t numchaps,
									git_time_t modified) {
	CHAPTER_NAME(numchaps, latest);
	void derp(string title, string description, string source) {
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
