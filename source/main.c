#define _GNU_SOURCE // O_PATH

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
#include <errno.h> // ENOENT


static bool AISNEWER(struct stat a, struct stat b) {
	if(a.st_mtime > b.st_mtime) return true;
	if(a.st_mtime == b.st_mtime) 
		return a.st_mtim.tv_nsec > b.st_mtim.tv_nsec;
	return false;
}


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

		// we can spare two file descriptors, to track the directories
		// use openat from there.

		int descend(int loc, const string name, bool create) {
		// XXX: ensure these are all null terminated, because open sucks
			assert(name.s[name.l] == '\0');
			int sub = openat(loc,name.s,O_DIRECTORY|O_PATH);
			if(!create) {
				ensure_gt(sub,0);
			} else if(sub < 0) {
				ensure_eq(errno,ENOENT);
				// mkdir if not exists is a painless operation
				ensure0(mkdirat(loc,name.s,0755));
				sub = openat(loc,name.s,O_DIRECTORY|O_PATH);
				ensure_gt(sub,0);
			}
			close(loc);
			return sub;
		}
		int destloc = descend(AT_FDCWD, category, true);
		destloc = descend(destloc, location, true);
		int srcloc = descend(AT_FDCWD, location, false);
		{ string markup = {LITLEN("markup")};
			srcloc = descend(srcloc, markup, false);
		}
		// don't forget to close these!

		bool skip(const char* srcname, const char* destname) {
			struct stat srcinfo, destinfo;
			bool dest_exists = (0==fstatat(destloc,destname,&destinfo,0));
			ensure0(fstatat(srcloc,srcname,&srcinfo,0));

			if(dest_exists) {
				if(AISNEWER(destinfo,srcinfo)) {
					// XXX: this will keep the db from getting chapter titles
					// if it's destroyed w/out deleting chapter htmls
					WARN("skip %s",srcname);
					return true;
				}
			} else {
				INFO("dest no exist %s",destname);
			}
			return false;
		}

		bool title_changed = false;
		bool numchaps_changed = false;
		{
			int countchaps = db_count_chapters(story);
			if(countchaps != numchaps) {
				numchaps_changed = true;
				WARN("#chapters changed %d -> %d",numchaps,countchaps);
				numchaps = countchaps;
			}
		}

		// save numchaps to update story later.
		const int savenumchaps = numchaps;
		// XXX: if finished, numchaps, otherwise
		if(!finished && numchaps > 1) --numchaps;

		void for_chapter(identifier chapter, git_time_t chapter_timestamp) {
			INFO("chapter %d", chapter);
			if(chapter == numchaps + 1) {
				// or other criteria, env, db field, etc
				WARN("not exporting last chapter");
				return;
			}

			const char* destname = "index.html";
			if(chapter > 1) {
				char buf[0x100];
				int amt = snprintf(buf,0x100, "chapter%d.html",chapter);
				assert(amt < 0x100);
				destname = buf;
			}
			char srcname[0x100];
			snprintf(srcname,0x100,"chapter%d.hish",chapter);

			if(skip(srcname,destname)) return;


			int src = openat(srcloc, srcname, O_RDONLY, 0755);
			ensure_ge(src,0);

			int dest = openat(destloc,".tempchap",O_WRONLY|O_CREAT|O_TRUNC,0644);
			ensure_ge(dest,0);

			create_chapter(src,dest,chapter,numchaps,story,&title_changed);
			ensure0(close(src));
			ensure0(close(dest));

			ensure0(renameat(destloc,".tempchap",destloc,destname));
		}

		// NOT story_timestamp
		db_for_chapters(story, for_chapter, timestamp);

		// we create contents.html strictly from the db, not the markup directory
		ensure0(close(srcloc));

		if(!(numchaps_changed || title_changed)) {
			// no chapters added or removed, and no chapter had an embedded title that changed.
			struct stat info;
			// be sure to create anyway if contents.html doesn't exist
			bool contents_exist = (0 == fstatat(destloc,"contents.html",&info,0));
			if(contents_exist) {
				WARN("not recreating contents of %d", story);
			}
			return;
		}

		/* be sure to create the contents after processing the chapters, to update the db
		 with any embedded chapter titles */
		
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

		int dest = openat(destloc,".tempcontents",O_WRONLY|O_CREAT|O_TRUNC,0644);
		ensure_ge(dest,0);
		create_contents(story, location, dest, numchaps, with_title);
		ensure0(close(dest));
		ensure0(renameat(destloc,".tempcontents",destloc,"contents.html"));
		ensure0(close(destloc));

		if(numchaps_changed) {
			db_set_chapters(story,savenumchaps);
		}
		// but if only the title of a chapter changed, we still recreate contents
	}

	INFO("stories since %d",timestamp);
	db_for_stories(for_story, timestamp);
	db_caught_up();
	INFO("caught up");
	db_close_and_exit();
	return 0;
}
