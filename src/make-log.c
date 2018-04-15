#include "itoa.h"
#include "storygit.h"
#include "repo.h"
#include "ensure.h"
#include "db.h"
#include "mystring.h"


#include <git2/revparse.h>

#include <sys/stat.h>
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

	const char* destname = argv[1];
	size_t destlen = strlen(destname);
	char* temp = malloc(destlen+5);
	memcpy(temp,destname,destlen);
	memcpy(temp+destlen,".tmp\0",5);
	FILE* dest = fopen(temp,"wt");
	bool got_modified = false;
	time_t last_modified = 0;
#define output_literal(lit) fwrite(LITLEN(lit),1,dest)
#define output_buf(s,l) fwrite(s,l,1,dest)

	void on_chapter(identifier story,
									size_t chapnum,
									const string story_title,
									const string chapter_title,
									const string location,
									git_time_t timestamp) {
		if(!got_modified) {
			got_modified = true;
			last_modified = timestamp;
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

	if(got_modified) {
		struct timespec times[2] = {
			{ .tv_sec = last_modified },
			{ .tv_sec = last_modified }
		};
		futimens(fileno(dest), times);
	}
	fclose(dest);
		
	
	return 0;
}
