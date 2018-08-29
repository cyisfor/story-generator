#define _GNU_SOURCE // O_PATH

#include "o/category.gen.h"

#include "storydb.h"
#include "ensure.h"
#include "storygit.h"
#include "storydb.h"
#include "string.h"
#include "repo.h"
#include "create.h"
#include "note.h"

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

struct csucks {
	size_t num;
	size_t counter;
	int chapspercom;
	git_time_t timestamp;
	bool any_chapter;
	git_time_t max_timestamp;
	bool title_changed;
	string scategory;
	bool only_story;
	int ready;
	bool adjust_times;
	int destloc;
	int srcloc;
	size_t numchaps;
	identifier story;
	bool fixing_srctimes;
	string location;
	int contents;
};

#define GDERP struct csucks* g = (struct csucks*)udata

bool skip(struct csucks* g, git_time_t srcstamp, const char* destname) {
	struct stat destinfo;
	bool dest_exists = (0==fstatat(g->destloc,destname,&destinfo,0));
	if(dest_exists) {
		if(destinfo.st_mtime >= srcstamp) {
			// XXX: this will keep the db from getting chapter titles
			// if it's destroyed w/out deleting chapter htmls
			//WARN("skip %s %d %d",destname,destinfo.st_mtime, srcstamp);
			return true;
		}
	} else {
		INFO("dest no exist %s",destname);
	}
	INFO("generating %s",destname);
	return false;
}


void for_chapter(
	void* udata,
	identifier chapter,
	git_time_t chapter_timestamp) {
	GDERP;
	SPAM("chapter %.*s%d %d",
			 g->location.l,g->location.s,
			 chapter,chapter_timestamp - g->timestamp);
	g->any_chapter = true;
	//SPAM("chap %d:%d\n",chapter,chapter_timestamp);
	if(chapter_timestamp > g->max_timestamp)
		g->max_timestamp = chapter_timestamp;
	// this should be moved later...
	char srcname[0x100];
	struct stat srcinfo;
	int src;

	bool setupsrc(void) {
		snprintf(srcname,0x100,"chapter%ld.hish",chapter);
		src = openat(g->srcloc, srcname, O_RDONLY, 0755);
		ensure_ge(src,0);
		// for adjusting dest timestamp
		ensure0(fstatat(g->srcloc,srcname,&srcinfo,0));
		if(srcinfo.st_mtime > chapter_timestamp) {
			// git ruins file modification times... we probably cloned this, and lost
			// all timestamps. Just set the source file to have changed with the commit then.
			srcinfo.st_mtime = chapter_timestamp;
			return true;
		}
		return false;
	}

	if(g->fixing_srctimes) {
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

	if(!storydb_all_ready && (chapter == g->numchaps + 1)) {
		// or other criteria, env, db field, etc
		WARN("not exporting last chapter");
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

	if(skip(g, chapter_timestamp,destname)) {
		if(g->adjust_times) {
			// mleh
			int dest = openat(g->destloc,destname,O_WRONLY);
			if(dest >= 0) {
				close_with_time(dest,chapter_timestamp);
			}
		}
	}

	int dest = openat(g->destloc,".tempchap",O_WRONLY|O_CREAT|O_TRUNC,0644);
	ensure_ge(dest,0);

	if(!g->fixing_srctimes)
		setupsrc();

	create_chapter(src,dest,chapter,g->numchaps,g->story,&g->title_changed);

	ensure0(close(src));
	close_with_time(g->dest,chapter_timestamp);

	ensure0(renameat(g->destloc,".tempchap",g->destloc,destname));
}

void on_title_for_contents(
	void* udata,
	void(*handle)(const string title),
	const string title) {
	// we temporarily have the title from the database.
	GDERP;
	if(title.s == NULL) {
		char buf[0x100];
		string fallback = {
			.s = buf,
			.l = snprintf(buf,0x100,"Chapter %lu",chapter)
		};
		create_contents(g->story, g->location, g->contents, g->numchaps, fallback);
	} else {
		create_contents(g->story, g->location, g->contents, g->numchaps, title);
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
	//DEBUG("story %lu %lu %.*s",story,numchaps,location.l,location.s);

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

	CLOSING int srcloc = descend(AT_FDCWD, g->location, false);
	g->srcloc = srcloc;
	if(srcloc < 0) return; // glorious moved
	{ string markup = {LITLEN("markup")};
		srcloc = descend(srcloc, markup, false);
		if(srcloc < 0) {
			WARN("ehhh... something got their markup directory removed? %.*s",
					 g->location.l, g->location.s);
			return;
		}
	}
	CLOSING int destloc = descend(AT_FDCWD, g->scategory, true);
	destloc = descend(destloc, g->location, true);

	g->title_changed = false;
	bool numchaps_changed = false;
	{
		int countchaps = storydb_count_chapters(story);
		if(countchaps != numchaps) {
			numchaps_changed = true;
			WARN("#chapters changed %d -> %d",numchaps,countchaps);
			/* if we've reduced our chapters, mark the new last chapter as seen recently
				 before it needs its next link removed
				 if we've increased our chapters, need to add the next link, or
				 create if it was the previous last chapter.
			*/
			storydb_saw_chapter(false,g->story,story_timestamp,countchaps);
			numchaps = countchaps;
		}
	}

	// save numchaps to update story later.
	const int savenumchaps = numchaps;
	// XXX: reduce numchaps to ready, unless all are ready
	if(!storydb_all_ready && ready)
		numchaps = ready; // + 1 ?

	git_time_t max_timestamp = story_timestamp;

	g->any_chapter = false; // stays false when no chapters are ready
	g->destloc = destloc;

	// NOT story_timestamp

	storydb_for_chapters(
		udata,
		for_chapter,
		story,
		g->timestamp,
		numchaps,
		storydb_all_ready);

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

	if(!(numchaps_changed || g->title_changed)) {
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
					close_with_time(dest, max_timestamp);
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

	WARN("recreating contents of %d", story);

	/* be sure to create the contents after processing the chapters, to update the db
		 with any embedded chapter titles */


	g->contents = openat(destloc,".tempcontents",O_WRONLY|O_CREAT|O_TRUNC,0644);
	ensure_ge(g->contents,0);
	storydb_with_chapter_title((void*)g, on_title_for_contents, story, chapter);

	close_with_time(g->contents,max_timestamp);
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
		WARN("huh? lots of chapters in this commit...");
	}
//			INFO("%d saw %d of %.*s",timestamp, chapnum,loc.l,loc.s);

	// XXX: todo: handle if unreadable
	printf("%ld saw %ld of ",g->timestamp, chapnum);
	STRPRINT(loc);
	fputc('\n',stdout);
	struct stat derp;
	if(0!=stat(src.s,&derp)) {
		WARN("%s wasn't there",src.s);
		return GFC_CONTINUE;
	}
	storydb_saw_chapter(deleted,
											storydb_get_story(loc, g->timestamp),
											g->timestamp,chapnum);

	return GFC_CONTINUE;
}


enum gfc_action on_commit(
	void* udata,
	const db_oid oid,
	const db_oid parent,
	git_time_t timestamp, 
	git_tree* last,
	git_tree* cur)
{	
	db_saw_commit(timestamp, oid);
	assert(last != NULL);

	GDERP;
	
	INFO("commit %d %d",timestamp, ++g->counter);
	g->chapspercom = 0;
	
	enum gfc_action ret = git_for_chapters_changed(g,on_chapter,last,cur);
	
	return ret;
}

int main(int argc, char *argv[])
{

	note_init();
	libxmlfixes_setup();
	struct stat info;
	// make sure we're outside the code directory
	while(0 != stat("code",&info)) ensure0(chdir(".."));
	repo_check(repo_discover_init(LITLEN(".git")));
	storydb_open();

	LIBXML_TEST_VERSION;

	xmlSetStructuredErrorFunc(NULL,cool_xml_error_handler);

	create_setup();

	INFO("searching...");

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
		repo_check(git_revparse_single(&thing1, repo, getenv("after")));
		ensure_eq(git_object_type(thing1),GIT_OBJ_COMMIT);
		git_commit* thing2 = (git_commit*)thing1;
		const git_oid* oid = git_commit_id(thing2);
		INFO("going back after commit %.*s",GIT_OID_HEXSZ,git_oid_tostr_s(oid));
		after = git_author_time(thing2);
		git_object_free(thing1);
	} else {
		db_last_seen_commit(&results,&after,before);
		if(results.after)
			INFO("going back after %d",after);

		if(results.before)
			INFO("current commit %.*s",2*sizeof(db_oid),db_oid_str(before));
	}

	struct csucks g = {
		.num = 0,
		.counter = 0
	};
	
	git_for_commits(on_commit, (void*)&g, results.after, after, results.before, before);
	// check this even when after is set, so we don't clear a before in progress?
	if(g.num > 0) {
		db_caught_up_committing();
		//putchar('\n');
	}

	db_retransaction();

	INFO("processing...");

	bool fixing_srctimes = getenv("fix_srctimes")!=NULL;

	bool only_ready = false;

	const string get_category() {
		if(getenv("category")!=NULL) {
			string c = {getenv("category")};
			switch(lookup_category(c.s)) {
			case CATEGORY_censored:
				//WARN("censored is a special category. set censored=1 instead plz");
				setenv("censored","1",1);
				c.l = LITSIZ("censored");
				storydb_only_censored = true;
				break;
			case CATEGORY_sneakpeek:
				storydb_all_ready = true;
				INFO("sneak peek!");
				c.l = LITSIZ("sneakpeek");
				break;
			case CATEGORY_ready:
				only_ready = true;
				c.l = LITSIZ("ready");
				break;
			};
			c.l = strlen(c.s);
			return c;
		} else if(getenv("censored")!=NULL) {
			storydb_only_censored = true;
			return (const string){LITLEN("censored")};
		} else {
			return (const string){LITLEN("html")};
		}
	}
	g.scategory = get_category();
	identifier category = db_get_category(g.scategory, &g.timestamp);
	if(getenv("recheck")) {
		g.timestamp = 0;
	} else if(getenv("before")) {
		long res = strtol(getenv("before"),NULL,0);
		if(res) {
			g.timestamp = res;
		}
	} else if(results.after) {
		g.timestamp = after;
	}

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
	INFO("stories after %d",g.timestamp);
	storydb_for_stories(&g, for_story, true, g.timestamp);
	db_caught_up_category(category);
	INFO("caught up");
	db_close_and_exit();
	return 0;
}
