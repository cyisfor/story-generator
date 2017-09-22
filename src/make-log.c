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
	db_open("generate.sqlite");

	struct storycache* cache = db_start_storycache();
	
	write(1,LITLEN(
					"<!DOCTYPE html>\n"
					"<html>\n"
					"<head>\n"
					"<meta charset=utf-8/>\n"
					"<link rel=stylesheet href=log.css />\n"
					"<title>Recent updates.</title>\n"
					"</head>\n<body>\n"
					"<table class=chaps>\n"
					));

	void on_chapter(identifier story,
									size_t chapnum,
									const string title,
									const string location,
									git_time_t timestamp) {

		if(db_in_storycache(cache,story,chapnum)) return;
		
		write(1,LITLEN("<tr><td>"));

		char num[0x10];
		int numlen = snprintf(num,0x100, "%d",chapnum);
		write(1, num, numlen);

		write(1,LITLEN("</td><td><a href=\""));
		write(1,location.s,location.l);
		write(1,LITLEN("/"));

		if(chapnum == 1) {
			write(1,LITLEN("index.html"));
		} else {
			write(1,LITLEN("chapter"));
			write(1,num, numlen);
			write(1,LITLEN(".html"));
		}
		write(1,LITLEN("\">"));

		if(title.s)
			write(1,title.s,title.l);
		else
			write(1,location.s,location.l);

		write(1, LITLEN("</a></td><td>"));

		char* s = ctime(&timestamp);
		write(1,s,strlen(s)-1);

		write(1,LITLEN("</td></tr>\n"));
	}

	db_for_recent_chapters(100, on_chapter);

	write(1,LITLEN("</table></body></html>"));
	
	return 0;
}
