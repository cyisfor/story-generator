#include "storygit.h"
#include "repo.h"
#include "ensure.h"
#include "db.h"

#include <git2/revparse.h>

#include <sys/stat.h>
#include <unistd.h> // chdir, write
#include <assert.h>

int main(int argc, char *argv[])
{
	struct stat info;
	while(0 != stat("code",&info)) ensure0(chdir(".."));
	db_open("generate.sqlite");

	struct storycache* cache = db_start_storycache();
	
	db_all_finished = getenv("sneakpeek") != NULL;
	db_only_censored = getenv("censored") != NULL;

#define output_literal(lit) write(1,LITLEN(lit))

	void on_chapter(identifier story,
									size_t chapnum,
									const string story_title,
									const string chapter_title,
									const string location,
									git_time_t timestamp) {

		if(db_in_storycache(cache,story,chapnum)) {
			write(2,LITLEN("yayayay"));
			return;
		}
		if(!db_all_finished) {
			if(db_count_chapters(story) == chapnum) return;
		}

		char num[0x10];
		int numlen = snprintf(num,0x100, "%d",chapnum+1);

		output_literal("<tr><td><a href=\"");
		write(1,location.s,location.l);
		output_literal("/");

		if(chapnum == 1) {
			output_literal("index.html");
		} else {
			output_literal("chapter");
			write(1,num, numlen);
			output_literal(".html");
		}
		output_literal("\">");

		void wrstory(void) {
			if(story_title.s)
				write(1,story_title.s,story_title.l);
			else
				write(1,location.s,location.l);
		}

		if(chapter_title.s) {
			write(1,chapter_title.s,chapter_title.l);
			output_literal(" (");
			wrstory();
			output_literal(")");
		} else {
			wrstory();
//			output_literal(" (chapter ");
//			write(1,num,numlen);
//			output_literal(")");
		}

		output_literal("</a></td><td>");

		write(1, num, numlen);

		output_literal("</td><td>\n");

		char* s = ctime(&timestamp);
		write(1,s,strlen(s)-1);

		output_literal("</td></tr>\n");
	}

	void output_body(void) {
		db_for_recent_chapters(10000, on_chapter);
	}

#include "o/template/make-log.html.c"

	db_storycache_free(cache);
	
	return 0;
}
