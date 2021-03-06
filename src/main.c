#define _GNU_SOURCE // O_PATH

#include "category.gen.h"

#include "storydb.h"
#include "ensure.h"
#include "storygit.h"
#include "storydb.h"
#include "string.h"
#include "repo.h"
#include "create.h"
#include "record.h"

#include "libxmlfixes/src/libxmlfixes.h"

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
	if(*fd == -1) return;
	ensure0(close(*fd));
	*fd = -1;
}

#define CLOSING __attribute__((__cleanup__(close_ptr)))

static bool AISNEWER(struct stat a, struct stat b) {
	if(a.st_mtime > b.st_mtime) return true;
	if(a.st_mtime == b.st_mtime)
		return a.st_mtim.tv_nsec > b.st_mtim.tv_nsec;
	return false;
}

/* isn't C a wonderful language? */

#define UTIME_THINGY(ACTION) struct timespec times[2] = { \
		{ .tv_sec = timestamp,																\
			.tv_nsec = 0																				\
		},																										\
		{ .tv_sec = timestamp,																\
			.tv_nsec = 0																				\
		}																											\
	};																											\
	if(0!=ACTION) {																					\
		perror("setting utime thingy");												\
		abort();																							\
	}

void set_timestamp_at(int location, const char* name, git_time_t timestamp) {
	UTIME_THINGY(utimensat(location, name, times, 0));
}

void close_with_time(int* dest, git_time_t timestamp) {
	UTIME_THINGY(futimens(*dest,times));
	close_ptr(dest);
}

static
void output_time_f(const char* msg, time_t time) {
	char buf[0x1000];
	struct tm tm = {};
	size_t len = strftime(buf, 0x1000, "%c", localtime_r(&time,&tm));

	record(INFO, msg,time,len, buf);
}

#define output_time(msg, time) output_time_f(msg " %d %.*s", time)

struct csucks {
	size_t num;
	size_t counter;
	int chapspercom;
	git_time_t timestamp;
	bool any_chapter;
	git_time_t max_timestamp;
	bool title_changed;
	string scategory;
	identifier only_story;
	int ready;
	bool adjust_times;
	int destloc;
	int srcloc;
	size_t numchaps;
	identifier story;
	bool fixing_srctimes;
	string location;
	git_time_t after;
};

#define GDERP struct csucks* g = (struct csucks*)udata

bool skip(struct csucks* g, git_time_t srcstamp, const char* destname) {
	struct stat destinfo;
	bool dest_exists = (0==fstatat(g->destloc,destname,&destinfo,0));
	if(dest_exists) {
		if(destinfo.st_mtime >= srcstamp) {
			// XXX: this will keep the db from getting chapter titles
			// if it's destroyed w/out deleting chapter htmls
			//record(WARNING, "skip %s %d %d",destname,destinfo.st_mtime, srcstamp);
			return true;
		}
	} else {
		record(INFO, "dest no exist %s",destname);
	}
	record(INFO, "generating %s",destname);
	return false;
}

static
const char* myctime(git_time_t* stamp) {
	char* c = ctime(stamp);
	c[strlen(c)-1] = '\0';
	return c;
}

void for_chapter(
	void* udata,
	identifier chapter,
	git_time_t chapter_timestamp) {
	GDERP;
	record(DEBUG, "chapter %.*s %ld timestamp %d %s",
			 g->location.len,g->location.base,
			 chapter,chapter_timestamp, myctime(&chapter_timestamp));
	g->any_chapter = true;
	//record(DEBUG, "chap %d:%d\n",chapter,chapter_timestamp);
	if(chapter_timestamp > g->max_timestamp)
		g->max_timestamp = chapter_timestamp;
	// this should be moved later...
	char srcname[0x100];
	struct stat srcinfo;
	int src;

	bool setupsrc(void) {
		snprintf(srcname,0x100,"chapter%ld.hish",chapter);
		src = openat(g->srcloc, srcname, O_RDONLY, 0755);
		if(src < 0) return false;

		// for adjusting dest timestamp
		ensure0(fstat(src,&srcinfo));

		if(srcinfo.st_mtime > chapter_timestamp) {
			// git ruins file modification times... we probably cloned this, and lost
			// all timestamps. Just set the source file to have changed with the commit then.
			return true;
		} else if(g->fixing_srctimes && chapter_timestamp > srcinfo.st_mtime) {
			// WHY
			// git ruins file modification times in its own database
			// randomly pulling this one out of its ass
			record(INFO, "chapter %d had bad git timestamp %d (->%d)",
					 chapter, chapter_timestamp, srcinfo.st_mtime);			
			chapter_timestamp = srcinfo.st_mtime;
			storydb_saw_chapter(false,g->story,chapter_timestamp,chapter);
		}
		return false;
	}

	if(g->fixing_srctimes) {
		if(setupsrc()) {
			struct timespec times[2] = {
				{
					.tv_sec = chapter_timestamp,
				},
				{
					.tv_sec = chapter_timestamp
				}
			};
			record(INFO, "chapter %d had bad timestamp %d (->%d)",
					 chapter, srcinfo.st_mtime, chapter_timestamp);
			output_time("file",srcinfo.st_mtime);
			output_time("db",chapter_timestamp);
			srcinfo.st_mtime = chapter_timestamp;
			ensure0(futimens(src,times));
		}
	}

	if(!storydb_all_ready && (chapter == g->numchaps + 1)) {
		// or other criteria, env, db field, etc
		record(WARNING, "not exporting last chapter");
		if(chapter > 2 && !storydb_all_ready && g->ready > 0) {
			// two chapters before this needs updating, before it now has a "next" link
			storydb_saw_chapter(false,g->story,chapter_timestamp,chapter-2);
		}
		return;
	}

	char destname[0x100] = "index.html";
	if(chapter > 1) {
		int amt = snprintf(destname,0x100, "chapter%ld.html",chapter);
		assert(amt < 0x100);
		/* be sure to mark the previous chapter as "seen" if we are the last chapter
			 being exported (previous needs a "next" link) */
		if(chapter == g->numchaps) {
			storydb_saw_chapter(false,g->story,chapter_timestamp,chapter-1);
		}
	}

	bool skipping = false;
	if(skip(g, chapter_timestamp,destname)) {
		record(DEBUG, "SKIP %s", myctime(&chapter_timestamp));
		if(g->adjust_times) {
			// mleh
			set_timestamp_at(g->destloc, destname, chapter_timestamp);
		}
		if(getenv("fix_titles")==NULL) {
			return;
		}
		skipping = true;
	}

	int dest;
	if(skipping) {
		dest = -1;
	} else {
		dest = openat(g->destloc,".tempchap",O_WRONLY|O_CREAT|O_TRUNC,0644);
		ensure_ge(dest,0);
	}

	if(!g->fixing_srctimes)
		setupsrc();

	create_chapter(src,dest,chapter,g->ready,g->story,&g->title_changed);

	close_ptr(&src);
	if(dest >= 0) {
		close_with_time(&dest,chapter_timestamp);
		ensure0(renameat(g->destloc,".tempchap",g->destloc,destname));
	}
}


void for_story(
	void* udata,
	identifier story,
	const string location,
	size_t ready,
	size_t numchaps,
	git_time_t story_timestamp) {

	GDERP;
	
	g->ready = ready;
	g->story = story;
	g->numchaps = numchaps;
	g->location = location;
	if(g->only_story != -1) {
		if(story != g->only_story) return;
	}
	//DEBUG("story %lu %lu %.*s",story,numchaps,location.len,location.base);

	// we can spare two file descriptors, to track the directories
	// use openat from there.

	int descend(int loc, const string name, bool create) {
		// XXX: ensure these are all null terminated, because open sucks
		assert(name.base[name.len] == '\0');
		int sub = openat(loc,name.base,O_DIRECTORY|O_PATH);
		if(!create) {
			return sub;
		} else if(sub < 0) {
			ensure_eq(errno,ENOENT);
			// mkdir if not exists is a painless operation
			ensure0(mkdirat(loc,name.base,0755));
			sub = openat(loc,name.base,O_DIRECTORY|O_PATH);
			ensure_gt(sub,0);
		}
		if(loc != AT_FDCWD)
			close_ptr(&loc);
		return sub;
	}

	CLOSING int srcloc = descend(AT_FDCWD, g->location, false);
	if(srcloc < 0) return; // glorious moved

	string title_file_derp(void) {
		struct stat info;
		if(0 == fstatat(srcloc, "title.png", &info, 0)) {
			return LITSTR("title.png");
		}
		return LITSTR("title.jpg");
	}
	const string title_file = title_file_derp();
	
	{ string markup = {LITLEN("markup")};
		srcloc = descend(srcloc, markup, false);
		if(srcloc < 0) {
			record(WARNING, "ehhh... something got their markup directory removed? %.*s",
					 g->location.len, g->location.base);
			return;
		}
	}
	g->srcloc = srcloc;

	CLOSING int destloc = descend(AT_FDCWD, g->scategory, true);
	destloc = descend(destloc, g->location, true);

	g->title_changed = false;
	bool numchaps_changed = false;
	{
		identifier countchaps = storydb_count_chapters(story);
		if(countchaps != numchaps) {
			numchaps_changed = true;
			record(WARNING, "#chapters changed %d -> %d",numchaps,countchaps);
			/* if we've reduced our chapters, mark the new last chapter
				 as seen recently since it needs its next link removed
				 if we've increased our chapters, also mark seen recently
				 since we need to add the next link, or create the previous
				 last chapter if it was skipped before.
			*/
			if(countchaps > 1) {
				storydb_saw_chapter(false,g->story,story_timestamp,countchaps-1);
			}
			g->numchaps = numchaps = countchaps;
		}
	}

	// save numchaps to update story later.
	const int savenumchaps = numchaps;
	// XXX: reduce numchaps to ready, unless all are ready
	if(!storydb_all_ready && ready) {
		g->numchaps = numchaps = ready; // + 1 ?
	}

	git_time_t max_timestamp = story_timestamp;

	g->any_chapter = false; // stays false when no chapters are ready
	g->destloc = destloc;

	// NOT story_timestamp

	storydb_for_chapters(
		udata,
		for_chapter,
		story,
		g->after,
		numchaps,
		storydb_all_ready);

	// we create contents.html strictly from the db, not the markup directory
	close_ptr(&srcloc);
	g->srcloc = -1;

	void destloc_done() {
		/* this is tricky. We can't "upgrade" destloc to a O_WRONLY,
			 but we can openat(destloc,".") sooo
		*/
		int wr = openat(destloc,".",O_DIRECTORY);
		close_ptr(&destloc);
		g->destloc = -1;
		ensure_ge(wr,0);
		close_with_time(&wr,max_timestamp);
	}

	if(NULL==getenv("recreate_contents") && !(numchaps_changed || g->title_changed)) {
		// no chapters added or removed, and no chapter had an embedded title that changed.
		struct stat info;
		// be sure to create anyway if contents.html doesn't exist
		bool contents_exist = (0 == fstatat(destloc,"contents.html",&info,0));
		if(contents_exist) {
			if(info.st_mtime >= max_timestamp) {
				// contents was created recently.
				// we don't have to recreate
				int dest = openat(destloc,"contents.html",O_WRONLY);
				if(dest >= 0) {
					close_with_time(&dest, max_timestamp);
				}
				destloc_done();
				return;
			}
		}
	}

	if(!g->any_chapter) {
		//DEBUG("not recreating contents of %d because no chapters",story);
		return;
	}

	record(WARNING, "recreating contents of %d", story);

	/* be sure to create the contents after processing the chapters, to update the db
		 with any embedded chapter titles */


	{
		int contents =
			openat(destloc,".tempcontents",O_WRONLY|O_CREAT|O_TRUNC,0644);
		ensure_ge(contents,0);
		create_contents(story, title_file,
						g->location, contents, g->numchaps);
		close_with_time(&contents,max_timestamp);
	}
	ensure0(renameat(destloc,".tempcontents",destloc,"contents.html"));

	destloc_done();
	if(numchaps_changed) {
		storydb_set_chapters(story,savenumchaps);
	}
	// but if only the title of a chapter changed, we still recreate contents
}

enum gfc_action on_chapter(
	void* udata,
	long int chapnum,
	bool deleted,
	const string loc,
	const string src)
{
	GDERP;
	if(++g->num % 100 == 0) {
		db_retransaction();
/*				putchar('\n');
					exit(23); */
	}
	if(++g->chapspercom == 5) {
		record(WARNING, "huh? lots of chapters in this commit...");
	}
//			record(INFO, "%d saw %d of %.*s",timestamp, chapnum,loc.len,loc.base);

	// XXX: todo: handle if unreadable
	record(INFO, "%ld saw %ld of ",g->timestamp, chapnum);
	fwrite(loc.base, loc.len, 1, stdout);
	fputc('\n',stdout);
	output_time("time",g->timestamp);
	struct stat derp;
	if(0!=stat(src.base,&derp)) {
		record(WARNING, "%s wasn't there",src.base);
		return GFC_CONTINUE;
	}
	storydb_saw_chapter(deleted,
											storydb_get_story(loc, g->timestamp),
											g->timestamp,chapnum);

	return GFC_CONTINUE;
}


enum gfc_action on_commit(
	void* udata,
	const struct git_item base,
	const struct git_item parent,
	int which_parent)
{
	if(which_parent == 0) {
		db_saw_commit(base.time, DB_OID(*base.oid));
	}
	db_saw_commit(parent.time, DB_OID(*parent.oid));
	assert(parent.tree != NULL);

	GDERP;
	
	record(INFO, "commit %d %d %d",base.time, which_parent, ++g->counter);
	output_time("time",base.time);
	g->chapspercom = 0;
	if(g->timestamp == 0) {
		g->timestamp = base.time;
	}
	enum gfc_action ret = git_for_chapters_changed(g,
																								 on_chapter,
																								 parent.tree,
																								 base.tree);
	
	return ret;
}

bool only_ready = false;


const string get_category() {
	if(getenv("category")!=NULL) {
		string c = {getenv("category")};
		c.len = strlen(c.base);
		switch(lookup_category(c.base,c.len)) {
		case CATEGORY_censored:
			//record(WARNING, "censored is a special category. set censored=1 instead plz");
			setenv("censored","1",1);
			c.len = LITSIZ("censored");
			storydb_only_censored = true;
			break;
		case CATEGORY_sneakpeek:
			storydb_all_ready = true;
			record(INFO, "sneak peek!");
			c.len = LITSIZ("sneakpeek");
			break;
		case CATEGORY_ready:
			only_ready = true;
			c.len = LITSIZ("ready");
			break;
		};
		c.len = strlen(c.base);
		return c;
	} else if(getenv("censored")!=NULL) {
		storydb_only_censored = true;
		return (const string){LITLEN("censored")};
	} else {
		return (const string){LITLEN("html")};
	}
}

int main(int argc, char *argv[])
{
	//libxmlfixes_setup();
	struct stat info;
	// make sure we're outside the code directory
	while(0 != stat("code",&info)) ensure0(chdir(".."));
	repo_check(repo_discover_init(LITLEN(".git")));
	storydb_open();

	LIBXML_TEST_VERSION;

	xmlSetStructuredErrorFunc(NULL,cool_xml_error_handler);

	create_setup();

	record(INFO, "searching...");

	// but not older than the last commit we dealt with
	struct bad results = {
		.after = false,
		.before = false
	};
	git_time_t after;
	db_oid before;
	BEGIN_TRANSACTION;
	if(getenv("after")) {
		results.after = true;
		git_object* thing1;
		int res = git_revparse_single(&thing1, repo, getenv("after"));
		switch(res) {
		case GIT_ENOTFOUND:
		case GIT_EAMBIGUOUS:
			after = atoi(getenv("after"));
			if(after) {
				results.after = true;
				output_time("(override) going back until",after);
			}
			break;
		case 0:
			ensure_eq(git_object_type(thing1),GIT_OBJECT_COMMIT);
			git_commit* thing2 = (git_commit*)thing1;
			const git_oid* oid = git_commit_id(thing2);
			after = git_author_time(thing2);
			after -= 100;
			record(INFO, "(override) going back until commit %.*s %d",GIT_OID_HEXSZ,git_oid_tostr_s(oid), after);
			git_object_free(thing1);
			break;
		default:
			repo_check(res);
		};
	} else {
		db_last_seen_commit(&results,&after,before);
		if(results.after) {
			after -= 100;
			output_time("going back until",after);
		}

		if(results.before) {
			record(INFO, "current commit %.*s",2*sizeof(db_oid),db_oid_str(before));
		}
	}

	struct csucks g = {
		.num = 0,
		.counter = 0
	};
	
	git_for_commits((void*)&g, on_commit, results.after, after, results.before, before);
	// check this even when after is set, so we don't clear a before in progress?
	if(results.after == true || g.num > 0) {

		// we didn't visit the most recent, since no diff from it?
		git_commit* latest = NULL;
		if(results.before) {
			repo_check(git_commit_lookup(&latest, repo, GIT_OID(before)));
			db_saw_commit(git_commit_time(latest), before);
			git_commit_free(latest);
			db_caught_up_committing();
		}
		//putchar('\n');
	}

	db_retransaction();

	record(INFO, "processing...");

	g.fixing_srctimes = getenv("fix_srctimes")!=NULL;

	g.scategory = get_category();
	identifier category = db_get_category(g.scategory, &g.timestamp);
	assert(g.timestamp != 0);
	if(getenv("recheck")) {
		g.after = 0;
	} else if(getenv("before")) {
		long res = strtol(getenv("before"),NULL,0);
		if(res) {
			g.after = res;
		}
	} else if(results.after) {
		g.after = after;
	}

//	g.timestamp -= 100;

	g.adjust_times = getenv("adjust_times")!=NULL;

	g.only_story = -1;
	if(getenv("story")) {
		string s = {
			getenv("story"),
			strlen(getenv("story"))
		};
		g.only_story = storydb_find_story(s);
	}

	db_retransaction();
	output_time("stories after",g.after);
	storydb_for_stories(&g, for_story, true, g.after);
	db_retransaction();
	db_caught_up_category(category);
	record(INFO, "all done");
	db_close_and_exit();
	return 0;
}
