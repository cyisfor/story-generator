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

		char numbuf[0x10];
		string num = {
			.s = numbuf,
			.l = itoa(numbuf,0x10,chapnum+1)
		};

		const string stitle = story_title.s ? story_title : location;
		const string ctitle = chapter_title.s ? chapter_title : stitle;

		char numbuf[0x10];
    string num = { .s = numbuf,
									 .l = itoa(num.s,0x10)
		};
		// need num even for chapnum 0, because "chapter" column

#include "o/template/make-log.row.c"
	}

	void output_body(void) {
		db_for_recent_chapters(10000, on_chapter);
	}

#include "o/template/make-log.html.c"

	db_storycache_free(cache);
	
	return 0;
}
