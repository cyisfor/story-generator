#include "git.h"
#include "repo.h"
#include <stdlib.h> // mergesort, NULL


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
	long int chapnum;
	string location;
};

struct chapter* chapters = NULL;
size_t nchap = 0;
size_t schap = 0;

int compare_loc(const string key, const string*test) {
	

int main(int argc, char *argv[])
{
	struct stat info;
	// make sure we're outside the code directory
	while(0 != stat("code",&info)) chdir("..");
	repo_init(".");

	bool on_chapter(git_time_t timestamp,
									long int num,
									string loc,
									const char* name) {

		// lookup location
		const char** location = bsearch(location,locations,nloc,sizeof(*locations),
																		compare_loc
		
		if(nchap > 100) return false;
		
		return true;

	}
		char htmlname[0x100];
		snprintf(htmlname,0x100,"chapter%d.html"
		char* dest = build_path(NULL, "..", "html", location, htmlname);

	repo_discover_in
	return 0;
}
