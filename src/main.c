#define _GNU_SOURCE // O_PATH

#include "category.gen.h"

#include "db.h"
#include "ensure.h"
#include "storygit.h"
#include "string.h"
#include "repo.h"
#include "create.h"
#include "note.h"

#include "libxmlfixes/libxmlfixes.h"

#include <git2/revparse.h>


#include <bsd/stdlib.h> // mergesort
#include <string.h> // memcmp, memcpy
#include <sys/stat.h>
#include <unistd.h> // chdir, mkdir
#include <stdio.h>
#include <fcntl.h> // open, O_*
#include <assert.h>
#include <errno.h> // ENOENT

void close_ptr(int* fd) {
	close(*fd);
	*fd = -1;
}

#define CLOSING __attribute__((__cleanup__(close_ptr)))

static bool AISNEWER(struct stat a, struct stat b) {
	if(a.st_mtime > b.st_mtime) return true;
	if(a.st_mtime == b.st_mtime)
		return a.st_mtim.tv_nsec > b.st_mtim.tv_nsec;
	return false;
}


void close_with_time(int dest, git_time_t timestamp) {
	// so people requesting the HTML file get its ACTUAL update date.
	struct timespec times[2] = {
		{ .tv_sec = timestamp,
			.tv_nsec = 0
		},
		{ .tv_sec = timestamp,
			.tv_nsec = 0
		}
	};
	if(0!=futimens(dest,times)) {
		perror("futimens");
		abort();
	}
	ensure0(close(dest));
}

int main(int argc, char *argv[])
{
	note_init();
	struct stat info;
	// make sure we're outside the code directory
	while(0 != stat("code",&info)) chdir("..");
	repo_check(repo_discover_init(LITLEN(".git")));
	db_open("generate.sqlite");

	xmlSetStructuredErrorFunc(NULL,cool_xml_error_handler);

	create_setup();

	bool always_finished = false;
	size_t num = 0;
	size_t counter = 0;
	enum gfc_action on_commit(
		const db_oid oid,
		const db_oid parent,
		git_time_t timestamp, 
		git_tree* last,
		git_tree* cur)
	{	
		db_saw_commit(timestamp, oid);
		assert(last != NULL);

		INFO("commit %d %d %.*s",timestamp, ++counter, 2*sizeof(db_oid),db_oid_str(oid));
		int chapspercom = 0;

		enum gfc_action on_chapter(
			long int chapnum,
			bool deleted,
			const string loc,
			const string src)
		{
			if(++num % 100 == 0) {
				db_retransaction();
/*				putchar('\n');
					exit(23); */
			}
			if(++chapspercom == 5) {
				puts("huh? lots of chapters in this commit...");
			}
			//printf("saw %d of ",chapnum);
			//STRPRINT(loc);
			//fputc('\n',stdout);
			struct stat derp;
			if(0!=stat(src.s,&derp)) {
				WARN("%s wasn't there",src.s);
				return GFC_CONTINUE;
			}
			db_saw_chapter(deleted,db_get_story(loc,timestamp),timestamp,chapnum);
			return GFC_CONTINUE;
		}
		enum gfc_action ret = git_for_chapters_changed(last,cur,on_chapter);

		return ret;
	}

	INFO("searching...");

	// but not older than the last commit we dealt with
	struct bad results = {
		.last = false,
		.current = false
	};
	db_oid last_commit, current_commit;
	BEGIN_TRANSACTION;
	if(getenv("until")) {
		results.last = true;
		git_object* thing1;
		repo_check(git_revparse_single(&thing1, repo, getenv("until")));
		ensure_eq(git_object_type(thing1),GIT_OBJ_COMMIT);
		git_commit* thing2 = (git_commit*)thing1;

		memcpy(last_commit, git_object_id(thing1)->id,sizeof(db_oid));
		git_object_free(thing1);
		INFO("using commit %.*s",2*sizeof(db_oid),db_oid_str(last_commit));
		// this is needed, to jump start when a confusing merge loses the db's
		// last known commit
		//arrgh we need a timestamp too
		git_time_t timestamp = git_commit_time(thing2);
		db_saw_commit(timestamp,last_commit);
	} else {
		db_last_seen_commit(&results,last_commit,current_commit);
		if(results.last)
			INFO("last seen commit %.*s",2*sizeof(db_oid),db_oid_str(last_commit));

		if(results.current)
			INFO("current commit %.*s",2*sizeof(db_oid),db_oid_str(current_commit));

		git_for_commits(results.last ? last_commit : NULL,
										results.current ? current_commit : NULL,
										on_commit);
	}
	if(num > 0) {
		db_caught_up_commits();
		//putchar('\n');
	}
	END_TRANSACTION;

	INFO("processing...");

	bool fixing_srctimes = getenv("fix_srctimes")!=NULL;

	const string get_category() {
		if(getenv("category")!=NULL) {
			string c = {getenv("category")};
			c.l = strlen(c.s);
			switch(lookup_category(c.s)) {
			case CATEGORY_censored:
				WARN("censored is a special category. set censored=1 instead plz");
				setenv("censored","1",1);
			case CATEGORY_sneakpeek:
				always_finished = true;
			};
			return c;
		} else if(getenv("censored")!=NULL) {
			return (const string){LITLEN("censored")};
		} else {
			return (const string){LITLEN("html")};
		}
	}
	git_time_t timestamp;

	const string scategory = get_category();
	identifier category = db_get_category(scategory, &timestamp);
	if(getenv("recheck")) timestamp = 0;

	bool adjust_times = getenv("adjust_times")!=NULL;

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
				return sub;
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

		CLOSING int srcloc = descend(AT_FDCWD, location, false);
		if(srcloc < 0) return; // glorious moved
		{ string markup = {LITLEN("markup")};
			srcloc = descend(srcloc, markup, false);
			if(srcloc < 0) {
				WARN("ehhh... something got their markup directory removed? %s",location);
				return;
			}
		}
		CLOSING int destloc = descend(AT_FDCWD, scategory, true);
		destloc = descend(destloc, location, true);

		struct stat destinfo;
		bool skip(git_time_t srcstamp, const char* destname) {
			bool dest_exists = (0==fstatat(destloc,destname,&destinfo,0));
			if(dest_exists) {
				if(destinfo.st_mtime >= srcstamp) {
					// XXX: this will keep the db from getting chapter titles
					// if it's destroyed w/out deleting chapter htmls
					WARN("skip %s",destname);
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
				if(countchaps < numchaps) {
					/* if we've reduced our chapters, mark the new last chapter as seen recently
						 since it needs its next link removed
					*/
					db_saw_chapter(false,story,story_timestamp,countchaps);
				}
				numchaps = countchaps;
			}
		}

		// save numchaps to update story later.
		const int savenumchaps = numchaps;
		// XXX: if finished, numchaps, otherwise
		if(!always_finished && !finished && numchaps > 1) --numchaps;

		git_time_t max_timestamp = story_timestamp;

		void for_chapter(identifier chapter, git_time_t chapter_timestamp) {
			if(chapter_timestamp > max_timestamp)
				max_timestamp = chapter_timestamp;
			// this should be moved later...
			char srcname[0x100];
			struct stat srcinfo;
			int src;

			bool setupsrc(void) {
				snprintf(srcname,0x100,"chapter%d.hish",chapter);
				src = openat(srcloc, srcname, O_RDONLY, 0755);
				ensure_ge(src,0);
				// for adjusting dest timestamp
				ensure0(fstatat(srcloc,srcname,&srcinfo,0));
				if(srcinfo.st_mtime > chapter_timestamp) {
					// git ruins file modification times... we probably cloned this, and lost
					// all timestamps. Just set the source file to have changed with the commit then.
					srcinfo.st_mtime = chapter_timestamp;
					return true;
				}
				return false;
			}

			if(fixing_srctimes) {
				if(setupsrc()) {
					struct timespec times[2] = {
						srcinfo.st_mtim,
						srcinfo.st_mtim
					};
					INFO("chapter %d had bad timestamp %d (->%d)",
							 chapter, srcinfo.st_mtime, chapter_timestamp);
					ensure0(futimens(src,times));
				}
			}

			if(chapter == numchaps + 1) {
				// or other criteria, env, db field, etc
				WARN("not exporting last chapter");
				if(chapter > 2 && !always_finished && !finished) {
					// two chapters before this needs updating, since it now has a "next" link
					db_saw_chapter(false,story,chapter_timestamp,chapter-2);
				}
				return;
			}

			char destname[0x100] = "index.html";
			if(chapter > 1) {
				int amt = snprintf(destname,0x100, "chapter%d.html",chapter);
				assert(amt < 0x100);
				/* be sure to mark the previous chapter as "seen" if we are the last chapter
					 being exported (previous needs a "next" link) */
				if(chapter == numchaps) {
					db_saw_chapter(false,story,chapter_timestamp,chapter-1);
				}
			}

			if(skip(chapter_timestamp,destname)) {
				if(adjust_times) {
					// mleh
					int dest = openat(destloc,destname,O_WRONLY);
					if(dest >= 0) {
						close_with_time(dest,chapter_timestamp);
					}
				}
			}

			int dest = openat(destloc,".tempchap",O_WRONLY|O_CREAT|O_TRUNC,0644);
			ensure_ge(dest,0);

			if(!fixing_srctimes)
				setupsrc();

			create_chapter(src,dest,chapter,numchaps,story,&title_changed);

			ensure0(close(src));
			close_with_time(dest,chapter_timestamp);

			ensure0(renameat(destloc,".tempchap",destloc,destname));
		}

		// NOT story_timestamp
		db_for_chapters(story, for_chapter, timestamp);

		// we create contents.html strictly from the db, not the markup directory
		ensure0(close(srcloc));

		void destloc_done() {
			/* this is tricky. We can't "upgrade" destloc to a O_WRONLY,
				 but we can openat(destloc,".") sooo
			*/
			int wr = openat(destloc,".",O_DIRECTORY);
			ensure0(close(destloc));
			ensure_ge(wr,0);
			close_with_time(wr,max_timestamp);
		}

		if(!(numchaps_changed || title_changed)) {
			// no chapters added or removed, and no chapter had an embedded title that changed.
			struct stat info;
			// be sure to create anyway if contents.html doesn't exist
			bool contents_exist = (0 == fstatat(destloc,"contents.html",&info,0));
			if(contents_exist) {
				WARN("not recreating contents of %d", story);
				int dest = openat(destloc,"contents.html",O_WRONLY);
				if(dest >= 0) {
					close_with_time(dest, max_timestamp);
				}
				destloc_done();
				return;
			}
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

		close_with_time(dest,max_timestamp);
		ensure0(renameat(destloc,".tempcontents",destloc,"contents.html"));

		destloc_done();
		if(numchaps_changed) {
			db_set_chapters(story,savenumchaps);
		}
		// but if only the title of a chapter changed, we still recreate contents
	}

	INFO("stories since %d",timestamp);
	db_for_stories(for_story, timestamp);
	db_caught_up_category(category);
	INFO("caught up");
	db_close_and_exit();
	return 0;
}
