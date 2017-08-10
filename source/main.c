#include "db.h"
#include "ensure.h"
#include "storygit.h"
#include "string.h"
#include "repo.h"
#include "create.h"

#include <bsd/stdlib.h> // mergesort
#include <string.h> // memcmp, memcpy
#include <sys/stat.h>
#include <unistd.h> // chdir, mkdir
#include <stdio.h>
#include <fcntl.h> // open, O_*
#include <assert.h>

int main(int argc, char *argv[])
{
	struct stat info;
	// make sure we're outside the code directory
	while(0 != stat("code",&info)) chdir("..");
	repo_check(repo_discover_init(LITLEN(".git")));

	create_setup();

	bool on_commit(db_oid oid, git_time_t timestamp, git_tree* last, git_tree* cur) {
		if(last == NULL) return true;
		
		bool on_chapter(long int chapnum,
										bool deleted,
										const string loc,
										const string src) {

			printf("saw %d of ",chapnum);
			STRPRINT(loc);
			fputc('\n',stdout);
			db_saw_chapter(deleted,db_find_story(loc),timestamp,chapnum);
			return true;
		}
		git_for_chapters_changed(last,cur,on_chapter);
		db_saw_commit(timestamp, oid);
	}

	puts("searching...");

	// but not older than the last commit we dealt with
	git_oid last_commit;
	git_time_t timestamp = 0;
	if(db_last_seen_commit(DB_OID(last_commit),&timestamp)) {
		git_for_commits(&last_commit, on_commit);
	} else {
		git_for_commits(NULL, on_commit);
	}

	puts("processing...");

	void for_story(identifier story,
								 const string location,
								 size_t numchaps,
								 git_time_t timestamp) {
		printf("story %lu ",numchaps);
		STRPRINT(location);
		fputc('\n',stdout);
		
		string dest = {
			.l = LITSIZ("testnew/") + location.l + LITSIZ("/contents.html\0")
		};
		size_t dspace = dest.l;
		dest.s = malloc(dspace); // use malloc so we can realloc and use this as prefix for chaps
		memcpy(dest.s,LITLEN("testnew/\0"));
		mkdir(dest.s,0755); // just in case
		memcpy(dest.s+LITSIZ("testnew/"),location.s,location.l);
		dest.s[LITSIZ("testnew/") +  location.l] = '\0';
		mkdir(dest.s,0755); // just in case
		string storydest = {
			.s = dest.s,
			.l = LITSIZ("testnew/")+location.l + 1
		};

		memcpy(dest.s+LITSIZ("testnew/")+location.l,LITLEN("/contents.html\0"));
		create_contents(location, dest, numchaps);

		void for_chapter(int chapter, git_time_t timestamp) {
			char htmlnamebuf[0x100] = "index.html";
			string htmlname = {
				.s = htmlnamebuf,
				.l = LITSIZ("index.html")
			};
			printf("chapter %d\n", chapter);
			STRPRINT(chapter->location);
			fputc('\n',stdout);
		
			if(chapter > 1) {
				// XXX: index.html -> chapter2.html ehh...
				htmlname.l = snprintf(htmlname.s,0x100,"chapter%d.html",chapter->num+1);
			}
			// reuse dest, extend if htmlname is longer than contents.html plus nul
			size_t chap_dlen = storydest.l + htmlname.l + 1;
			if(dspace < chap_dlen) {
				dspace = ((chap_dlen>>8)+1)<<8;
				dest.s = realloc(dest.s, dspace);
			}
			memcpy(dest.s + storydest.l, htmlname.s, htmlname.l);
			dest.s[storydest.l + htmlname.l] = '\0';
			dest.l = storydest.l + htmlname.l + 1;

			string src = {
				.l = location.l + LITSIZ("/markup\0")
			};
			src.s = alloca(src.l);
			memcpy(src.s,location.s,location.l);
			memcpy(src.s + location.l, LITLEN("/markup\0"));
			
			create_chapter(src,dest,chapter,numchaps);
		}

		db_for_chapters(story, for_chapter, timestamp);
		
		free(dest.s);
	}

	db_for_stories(for_story, timestamp);

	return 0;
}
