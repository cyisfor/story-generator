#include "storygit.h"
#include "repo.h"
#include "ensure.h"
#include "db.h"

#include <git2/revparse.h>

#include <sys/stat.h>
#include <unistd.h> // chdir, write
#include <assert.h>

int main(int argc, char *argv[])
{
	struct stat info;
	while(0 != stat("code",&info)) ensure0(chdir(".."));
	repo_check(repo_discover_init(LITLEN(".git")));
	db_open("generate.sqlite");
	
	git_object* until;
	repo_check(git_revparse_single(&until, repo, "HEAD~1280"));
	ensure_eq(git_object_type(until),GIT_OBJ_COMMIT);

	write(1,LITLEN(
					"<!DOCTYPE html>\n"
					"<html>\n"
					"<head>\n"
					"<meta charset=utf-8/>\n"
					"<link rel=stylesheet href=log.css />\n"
					"<title>Recent updates.</title>\n"
					"</head>\n<body>\n"
					"<ul class=chaps>\n"
					));

	void on_chapter(identifier story,
									size_t chapnum,
									const string title,
									const string location,
									git_time_t timestamp) {
		write(1,LITLEN("<li><a href=\""));
		write(1,location.s,location.l);
		write(1,LITLEN("/"));

		char destname[0x100] = "index.html";
		int amt = LITSIZ("index.html");
		if(chapnum > 1) {
			amt = snprintf(destname,0x100, "chapter%d.html",chapnum);
			assert(amt < 0x100);
		}
		write(1,destname,amt);
		write(1,LITLEN("\" title=\""));
		char* s = ctime(&timestamp);
		write(1,s,strlen(s)-1);
		write(1,LITLEN("\">"));

		if(title.s)
			write(1,title.s,title.l);
		else
			write(1,location.s,location.l);
		
		write(1,LITLEN("</a></li>"));
	}

	db_for_recent_chapters(100, on_chapter);

	write(1,LITLEN("</ul></body></html>"));
	
	return 0;
}
