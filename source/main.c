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

	void for_story(const string location, int numchaps) {
		string dest = {
			.l = LITSIZ("testnew/") + location.l + LITSIZ("/contents.html\0")
		};
		dest.s = alloca(dest.l);
		memcpy(dest.s,LITLEN("testnew/\0"));
		mkdir(dest.s,0755); // just in case
		memcpy(dest.s+LITSIZ("testnew/"),location.s,location.l);
		dest.s[LITSIZ("testnew/") +  location.l] = '\0';
		mkdir(dest.s,0755); // just in case
		memcpy(dest.s+LITSIZ("testnew/")+locations.l,LITLEN("/contents.html\0"));
		create_contents(location, dest, numchaps);
	}
	
	puts("processing...");

	for(i=0;i<nchap;++i) {
		printf("chapter %d of %d (allstories)\n",i,nchap);
		char htmlnamebuf[0x100] = "index.html";
		string htmlname = {
			.s = htmlnamebuf,
			.l = LITSIZ("index.html")
		};
		const struct chapter* chapter = chapters + i;
		printf("%p chapter %d of ",chapter->location.s, chapter->num);
		STRPRINT(chapter->location);
		fputc('\n',stdout);
		
		if(chapter->num > 0) {
			// XXX: index.html -> chapter2.html ehh...
			htmlname.l = snprintf(htmlname.s,0x100,"chapter%d.html",chapter->num+1);
		}
		string dest = {
			.l = LITSIZ("html/") + chapter->location.l + LITSIZ("/") + htmlname.l + 3
		};
		dest.s = alloca(dest.l);
		memcpy(dest.s,LITLEN("testnew/"));
		memcpy(dest.s + LITSIZ("testnew/"), chapter->location.s, chapter->location.l);
		dest.s[LITSIZ("testnew/")+chapter->location.l] = '/';
		memcpy(dest.s+LITSIZ("testnew/")+chapter->location.l+1,htmlname.s,htmlname.l);
		dest.s[LITSIZ("testnew/")+chapter->location.l+1 + htmlname.l] = '\0';

		struct location* testloc = bsearch(&chapter->location,
																			 locations,nloc,sizeof(*locations),
																			 (void*)compare_loc);
		assert(testloc);
		create_chapter(src,dest,chapter->num,testloc->totalchaps);
		/* do NOT free(chapter->location.s); because it's interned. only free after ALL
			 chapters are done. */

	}

	return 0;
}
