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
	db_open("generderp.sqlite");

	create_setup();

	int derp = 0;

	size_t num = 0;
	bool on_commit(db_oid oid, git_time_t timestamp, git_tree* last, git_tree* cur) {
		if(last == NULL) {
			db_saw_commit(timestamp, oid);
			return true;
		}

		printf("commit %d %s\n",sizeof(db_oid),db_oid_str(oid));

		if(++derp == 100) return false;
		
		bool on_chapter(long int chapnum,
										bool deleted,
										const string loc,
										const string src) {
			if(++num % 100 == 0) db_retransaction();
			printf("saw %d of ",chapnum);
			STRPRINT(loc);
			fputc('\n',stdout);
			db_saw_chapter(deleted,db_find_story(loc,timestamp),timestamp,chapnum);
			return true;
		}
		bool ret = git_for_chapters_changed(last,cur,on_chapter);

		return ret;
	}

	puts("searching...");

	// but not older than the last commit we dealt with
	git_oid last_commit;
	git_time_t timestamp = 0;
	void intrans(void) {
		if(db_last_seen_commit(DB_OID(last_commit),&timestamp)) {
			printf("last seen commit %s\n",db_oid_str(DB_OID(last_commit)));
			git_for_commits(&last_commit, on_commit);
		}	else {
			git_for_commits(NULL, on_commit);
		}
	}
	db_transaction(intrans);

	puts("processing...");

	void for_story(identifier story,
								 const string location,
								 size_t numchaps,
								 git_time_t story_timestamp) {
		printf("story %lu ",numchaps);
		STRPRINT(location);
		fputc('\n',stdout);
		
		mstring dest = {
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

		void with_title(identifier chapter, void(*handle)(const string title)) {
			void on_title(const string title) {
				if(title.s == NULL) {
					char buf[0x100];
					string fallback = {
						.s = buf,
						.l = snprintf(buf,0x100,"Chapter %lu",chapter)
					};
					handle(fallback);
				} else {
					handle(title);
				}
			}
			db_with_chapter_title(story,chapter,on_title);
		}
		create_contents(location, CSTR(dest), numchaps, with_title);

		void for_chapter(identifier chapter, git_time_t chapter_timestamp) {
			char htmlnamebuf[0x100] = "index.html";
			mstring htmlname = {
				.s = htmlnamebuf,
				.l = LITSIZ("index.html")
			};
			printf("chapter %d\n", chapter);
		
			if(chapter > 1) {
				// XXX: index.html -> chapter2.html ehh...
				htmlname.l = snprintf(htmlname.s,0x100,"chapter%d.html",chapter);
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

			mstring src = {
				.l = location.l + LITSIZ("/markup\0")
			};
			src.s = alloca(src.l);
			memcpy(src.s,location.s,location.l);
			memcpy(src.s + location.l, LITLEN("/markup\0"));
			
			create_chapter(CSTR(src),CSTR(dest),chapter,numchaps);
		}

		// NOT story_timestamp
		db_for_chapters(story, for_chapter, timestamp);
		
		free(dest.s);
	}

	printf("stories since %d\n",timestamp);
	db_for_stories(for_story, timestamp);
	db_caught_up();
	return 0;
}
