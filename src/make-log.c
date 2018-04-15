#define _GNU_SOURCE // futimens

#include "itoa.h"
#include "storygit.h"
#include "repo.h"
#include "ensure.h"
#include "db.h"
#include "mystring.h"

#include "become.h"

#include <git2/revparse.h>

#include <sys/stat.h> // futimens
#include <unistd.h> // chdir
#include <assert.h>
#include <stdio.h> // fwrite

int main(int argc, char *argv[])
{
	assert(argc == 2);
	struct stat info;
	while(0 != stat("code",&info)) ensure0(chdir(".."));
	db_open();

	struct storycache* cache = db_start_storycache();
	
	db_all_finished = getenv("sneakpeek") != NULL;
	db_only_censored = getenv("censored") != NULL;

	struct becomer* dest = become_start(argv[1]);
#define output_literal(lit) fwrite(lit, LITSIZ(lit),1,dest->out)
#define output_buf(s,l) fwrite((s),(l),1,dest->out)

	void on_chapter(identifier story,
									size_t chapnum,
									const string story_title,
									const string chapter_title,
									const string location,
									git_time_t timestamp) {
		if(!dest->got_times) {
			dest->got_times = true;
			printf("lastdos %ld\n",timestamp);
			dest->times[0].tv_sec = timestamp;
			dest->times[1].tv_sec = timestamp;
		}
		if(db_in_storycache(cache,story,chapnum)) {
			fwrite(LITLEN("yayayay"),1,stderr);
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
		// need num even for chapnum 0, because "chapter" column
		
		const string stitle = story_title.s ? story_title : location;
		const string ctitle = chapter_title.s ? chapter_title : stitle;

#include "o/template/make-log.row.html.c"
	}

	void output_rows(void) {
		db_for_recent_chapters(10000, on_chapter);
	}

#include "o/template/make-log.html.c"

	db_storycache_free(cache);
	become_commit(&dest);
	
	return 0;
}
