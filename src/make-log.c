#define _GNU_SOURCE // futimens

#include "itoa.h"
#include "storygit.h"
#include "repo.h"
#include "ensure.h"
#include "storydb.h"
#include "mystring.h"

#include "become.h"

#include <git2/revparse.h>

#include <sys/stat.h> // futimens
#include <unistd.h> // chdir
#include <assert.h>
#include <stdio.h> // fwrite

struct becomer* dest = NULL;
struct storycache* cache = NULL;

#define output_literal(lit) fwrite(lit, LITSIZ(lit),1,dest->out)
#define output_buf(s,l) fwrite((s),(l),1,dest->out)

void on_chapter(void* udata,
								identifier story,
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
	if(storydb_in_cache(cache,story,chapnum)) {
		return;
	}
	if(!storydb_all_ready) {
		if(storydb_count_chapters(story) == chapnum) return;
	}

	char numbuf[0x10];
	string num = {
		.base = numbuf,
		.len = itoa(numbuf,0x10,chapnum)
	};
	// need num even for chapnum 0, because "chapter" column
		
	const string stitle = story_title.base ? story_title : location;
	const string ctitle = chapter_title.base ? chapter_title : stitle;

#include "template/make-log.row.html.c"
}


void output_rows(void) {
	storydb_for_recent_chapters(NULL, 10000, on_chapter);
}



int main(int argc, char *argv[])
{
	assert(argc == 2);
	struct stat info;
	while(0 != stat("code",&info)) ensure0(chdir(".."));
	storydb_open();

	cache = storydb_start_cache();
	
	storydb_all_ready = getenv("sneakpeek") != NULL;
	storydb_only_censored = getenv("censored") != NULL;

	dest = become_start(argv[1]);

#include "template/make-log.html.c"

	storydb_cache_free(cache);
	become_commit(&dest);
	
	return 0;
}
