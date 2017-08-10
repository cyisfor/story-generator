#include "git.h"
#include "repo.h"

int main(int argc, char *argv[])
{
	struct stat info;
	// make sure we're outside the code directory
	while(0 != stat("code",&info)) chdir("..");
	repo_init(".");

	bool on_chapter(git_time_t timestamp,
									long int num,
									const char* location,
									const char* name) {
		char htmlname[0x100];
		snprintf(htmlname,0x100,"chapter%d.html"
		char* dest = build_path(NULL, "..", "html", location, htmlname);

	repo_discover_in
	return 0;
}
