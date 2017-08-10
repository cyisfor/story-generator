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

string* locations = NULL;
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

	bool on_chapter(git_time_t timestamp,
									long int chapnum,
									const string loc,
									const string name) {

		/*printf("saw %d of ",chapnum);
		STRPRINT(loc);
		fputc('\n',stdout);*/
		// lookup location
		string* testloc = bsearch(&loc,locations,nloc,sizeof(*locations),
															 (void*)compare_loc);
		char* internkey;
		if(testloc == NULL) {
			if((nloc+1)<<1 >= sloc) {
				sloc = ((((nloc+1)>>7)+1)<<8);
				locations = realloc(locations,sizeof(*locations)*sloc);
			}
			locations[nloc].s = malloc(loc.l);
			internkey = locations[nloc].s;
			locations[nloc].l = loc.l;
			memcpy(locations[nloc].s, loc.s, loc.l);
			printf("CREATE LOCATION %p ",locations[nloc].s);
			STRPRINT(loc);
			putchar('\n');
			ensure0(mergesort(locations,++nloc,sizeof(*locations),(void*)compare_loc));
		} else {
			internkey = testloc->s;
		}

		// now location->s is our interned string key, combine with chapnum to make a unique key
		// note: this algorithm will process all chapter 1's, then all chapter 2's etc

		/* incidentally, since malloc always returns increasing values if nothing is freed,
			 the pointer order interned strings will be the order of most recently seen key.
			 a d b a a a c a b d -> a d b c
		*/

		int find_chapter(void* ignoredkey, struct chapter* value) {
			if(chapnum < value->num) return -1;
			if(chapnum == value->num)
				if(internkey < value->location.s) // magic
					return -1;
				else if(internkey == value->location.s)
					return 0;
			return 1;
		}

		struct chapter* chapter = bsearch(NULL,chapters,nchap,sizeof(*chapters),
																			(void*)find_chapter);
		if(chapter == NULL) {
			// if timestamp <= the earliest time we last went to...
			if(nchap > 2000) return false;
			if(nchap+1 >= schap) {
				schap = ((((nchap+1)>>7)+1)<<8);
				chapters = realloc(chapters,schap*sizeof(*chapters));
			}
			chapter = &chapters[nchap];
			chapter->num = chapnum;
			chapter->location.s = internkey; // NOT loc.s
			chapter->location.l = loc.l; // ...fine
			printf("CREATE CHAPTER %d %p ",chapter->num, internkey);
			STRPRINT(chapter->location);
			putchar('\n');

			ensure0(mergesort(chapters,++nchap,sizeof(*chapters),(void*)compare_chap));
		}
		
		return true;
	}

	puts("searching...");
	git_for_chapters(on_chapter);
	printf("%d locations found\n",nloc);
	
	puts("processing...");

	size_t i;
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
		memcpy(dest.s,LITLEN("testnew/\0"));
		mkdir(dest.s,0755); // just in case
		memcpy(dest.s + LITSIZ("testnew/"), chapter->location.s, chapter->location.l);
		dest.s[LITSIZ("testnew/") +  chapter->location.l] = '\0';
		mkdir(dest.s,0755); // just in case
		dest.s[LITSIZ("testnew/")+chapter->location.l] = '/';
		memcpy(dest.s+LITSIZ("testnew/")+chapter->location.l+1,htmlname.s,htmlname.l);
		dest.s[LITSIZ("testnew/")+chapter->location.l+1 + htmlname.l] = '\0';

		char namebuf[0x100];
		string name = {
			.s = namebuf,
		};
		name.l = snprintf(name.s,0x100,"chapter%d.hish",chapter->num+1);

		string src = {
			.l = chapter->location.l + LITSIZ("/markup/") + name.l + 1
		};
		// src/dest must be null terminated because open and stat both SUCK
		src.s = alloca(src.l);
		memcpy(src.s,chapter->location.s,chapter->location.l);
		memcpy(src.s + chapter->location.l,LITLEN("/markup/"));
		memcpy(src.s + chapter->location.l + LITSIZ("/markup/"), name.s, name.l);
		src.s[chapter->location.l + LITSIZ("/markup/") + name.l] = '\0';

		create_chapter(src,dest,chapter->num,9000);
		/* do NOT free(chapter->location.s); because it's interned. only free after ALL
			 chapters are done. */

	}

	return 0;
}
