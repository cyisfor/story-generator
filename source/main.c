#include "ensure.h"
#include "git.h"
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

struct location {
	char* s;
	short l;
	int totalchaps;
};

struct location* locations = NULL;
size_t nloc = 0;
size_t sloc = 0;

/* just to avoid too many strdups, check locations if exists, then use that. otherwise
	 strdup, insert into locations and sort, then use that.

	 but copy the string structure, so that the pointer .s is in chapter and locations.
	 then don't delete either until done with chapters. Then mass delete locations .s's

	 since .s is interned, can use the POINTER as a sorting key.
	 i.e. .s == 0x14554345 is always true for "somestory"
*/


struct chapter {
	long int num;
	string location;
};

struct chapter* chapters = NULL;
size_t nchap = 0;
size_t schap = 0;

static int compare_loc(const string* key, const string* test) {
	// arbitrary sort, go with quick algorithm: size, then strcmp
	if(key->l < test->l) return -1;
	if(key->l == test->l)
		return memcmp(key->s,test->s,key->l);
	return 1;
}

static int compare_chap(const struct chapter* key, const struct chapter* test) {
	// arbitrary sort, go with quick algorithm: num, then interned string pointer
	if(key->num < test->num) return -1;
	if(key->num == test->num) 
		if(key->location.s < test->location.s)
			return -1;
		else if(key->location.s == test->location.s)
			return 0;
	return 1;
}

int main(int argc, char *argv[])
{
	struct stat info;
	// make sure we're outside the code directory
	while(0 != stat("code",&info)) chdir("..");
	repo_check(repo_discover_init(LITLEN(".git")));

	create_setup();

	bool on_commit(git_time_t timestamp, git_tree* last, git_tree* cur) {
		if(last == NULL) return true;
		
		bool on_chapter(long int chapnum,
										bool deleted,
										const string loc,
										const string src) {

			printf("saw %d of ",chapnum);
			STRPRINT(loc);
			fputc('\n',stdout);
			db_saw_chapter(deleted,loc,chapnum);
			return true;
		}
		git_for_chapters_changed(last,cur,on_chapter);
	}

	puts("searching...");
	git_for_commits(on_commit);




	
	printf("%d locations found\n",nloc);

	int* chaptotal = malloc(sizeof(int*) * nloc);
	int i;
	for(i=0;i<nloc;++i) {
		string dest = {
			.l = LITSIZ("testnew/") + locations[i].l + LITSIZ("/contents.html\0")
		};
		dest.s = alloca(dest.l);
		memcpy(dest.s,LITLEN("testnew/\0"));
		mkdir(dest.s,0755); // just in case
		memcpy(dest.s+LITSIZ("testnew/"),locations[i].s,locations[i].l);
		dest.s[LITSIZ("testnew/") +  locations[i].l] = '\0';
		mkdir(dest.s,0755); // just in case
		memcpy(dest.s+LITSIZ("testnew/")+locations[i].l,LITLEN("/contents.html\0"));
		locations[i].totalchaps = create_contents(*((string*)locations+i), dest);
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
