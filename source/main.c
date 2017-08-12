#include "db.h"
#include "ensure.h"
#include "storygit.h"
#include "string.h"
#include "repo.h"
#include "create.h"
#include "note.h"

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

	size_t num = 0;
	bool on_commit(db_oid oid, git_time_t timestamp, git_tree* last, git_tree* cur) {
		db_saw_commit(timestamp, oid);
		if(last == NULL) {
			return true;
		}

		printf("commit %d %s\r",sizeof(db_oid),db_oid_str(oid));

		bool on_chapter(long int chapnum,
										bool deleted,
										const string loc,
										const string src) {
			if(++num % 100 == 0) db_retransaction();
			//printf("saw %d of ",chapnum);
			//STRPRINT(loc);
			//fputc('\n',stdout);
			struct stat derp;
			if(0!=stat(src.s,&derp)) {
				WARN("%s wasn't there\n",src.s);
				return true;
			}
			db_saw_chapter(deleted,db_find_story(loc,timestamp),timestamp,chapnum);
			return true;
		}
		bool ret = git_for_chapters_changed(last,cur,on_chapter);

		return ret;
	}

	INFO("searching...");

	// but not older than the last commit we dealt with
	struct bad results;
	db_oid last_commit, current_commit;
	git_time_t timestamp = 0;
	BEGIN_TRANSACTION(last_seen);
	db_last_seen_commit(&results,last_commit,current_commit,&timestamp);
	if(results.last) {
		const char* derp = db_oid_str(last_commit);
		INFO("last seen commit %.*s",2*sizeof(db_oid),derp);
		puts(derp);
	}
	if(results.current)
		INFO("current commit %.*s",2*sizeof(db_oid),db_oid_str(current_commit));
	git_for_commits(results.last ? last_commit : NULL,
									results.current ? current_commit : NULL,
									on_commit);
	END_TRANSACTION(last_seen);
	if(num > 0) putchar('\n');

	if(getenv("recheck")) timestamp = 0;

	string category = {LITLEN("html")};

	if(getenv("category")!=NULL) {
		category.s = getenv("category");
		category.l = strlen(category.s);
	} else if(getenv("censored")!=NULL) {
		category.s = "censored";
		category.l = LITSIZ("censored");
	}

	INFO("processing...");

	void for_story(identifier story,
								 const string location,
								 bool finished,
								 size_t numchaps,
								 git_time_t story_timestamp) {
		INFO("story %lu %lu %.*s",story,numchaps,location.l,location.s);
		
		mstring dest = {
			.l = category.l + LITSIZ("/") + location.l + LITSIZ("/contents.html\0")
		};
		size_t dspace = dest.l;
		dest.s = malloc(dspace); // use malloc so we can realloc and use this as category for chaps
		memcpy(dest.s,category.s,category.l);
		dest.s[category.l] = '\0';
		mkdir(dest.s,0755); // just in case
		dest.s[category.l] = '/';
		memcpy(dest.s+category.l+1,location.s,location.l);
		dest.s[category.l+1 +  location.l] = '\0';
		mkdir(dest.s,0755); // just in case
		string storydest = {
			.s = dest.s,
			.l = category.l+1+location.l + 1
		};

		memcpy(dest.s+category.l+1+location.l,LITLEN("/contents.html\0"));

		void dextend(const char* s, size_t len) {
			size_t dlen = storydest.l + len + 1;
			if(dspace < dlen) {
				dspace = ((dlen>>8)+1)<<8;
				dest.s = realloc(dest.s, dspace);
			}
			memcpy(dest.s + storydest.l, s, len);
			dest.s[storydest.l + len] = '\0';
			dest.l = storydest.l + len;
		}

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

		// XXX: if finished, numchaps, otherwise
x		if(!finished && numchaps > 1) --numchaps;
		create_contents(story, location, CSTR(dest), numchaps, with_title);

		// now we can mess with dest.s

		void for_chapter(identifier chapter, git_time_t chapter_timestamp) {
			INFO("chapter %d", chapter);
			if(chapter == numchaps + 1) {
				// or other criteria, env, db field, etc
				WARN("not exporting last chapter");
				return;
			}

			if(chapter == 1) {
				dextend(LITLEN("index.html"));
			} else {
				// XXX: index.html -> chapter2.html ehh...
				for(;;) {
					int amt = snprintf(dest.s+storydest.l,
														 dspace-storydest.l,
														 "chapter%d.html",chapter);
					if(amt + storydest.l + 1 > dspace) {
						dspace = (((amt+storydest.l)>>8)+1)<<8;
						dest.s = realloc(dest.s,dspace);
					} else {
						dest.l = storydest.l + amt;
						break;
					}
				}
			}
			mstring src = {
				.l = location.l + LITSIZ("/markup/chapterXXXXX.hish")
			};
			src.s = alloca(src.l);
			memcpy(src.s,location.s,location.l);
			memcpy(src.s + location.l, LITLEN("/markup/chapter"));
			int amt = snprintf(src.s + location.l + LITSIZ("/markup/chapter"),
												 src.l - (location.l + LITSIZ("/markup/chapter")),
												 "%d",chapter);
			assert(amt > 0);
			assert(src.l - (location.l + LITSIZ("/markup/chapter")) - amt > 5); // for .hish at end
			memcpy(src.s + location.l + LITSIZ("/markup/chapter") + amt,LITLEN(".hish\0"));
			src.l = location.l + LITSIZ("/markup/chapter") + amt + LITSIZ(".hish");
			create_chapter(CSTR(src),CSTR(dest),chapter,numchaps,story);
		}

		// NOT story_timestamp
		db_for_chapters(story, for_chapter, timestamp);
		
		free(dest.s);
	}

	INFO("stories since %d",timestamp);
	db_for_stories(for_story, timestamp);
	db_caught_up();
	INFO("caught up");
	db_close_and_exit();
	return 0;
}
