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
	repo_check(git_revparse_single(&until, repo, "HEAD~128"));
	ensure_eq(git_object_type(until),GIT_OBJ_COMMIT);

	write(1,LITLEN(
					"<!DOCTYPE html>\n"
					"<html>\n"
					"<head>\n"
					"<meta charset=utf-8/>\n"
					"<title>Recent updates.</title>\n"
					"</head>\n<body>\n"
					"<ul class=commits>\n"
					));
	
	bool on_commit(db_oid oid, git_time_t timestamp, git_tree* last, git_tree* cur) {
		if(last == NULL) {
			return true;
		}
		void start(void) {
			write(1,LITLEN("<li><div class=date>"));
			char* s = ctime(&timestamp);
			write(1,s,strlen(s));
			write(1,LITLEN("<li></div>\n<ul class=chaps>"));
		}
		bool started = false;

		bool on_chapter(long int chapnum,
										bool deleted,
										const string loc,
										const string src) {
			if(deleted) return true;
			identifier story = db_find_story(loc);
			int num = db_count_chapters(story);
			if(chapnum == num) return true;

			if(!started) {
				start();
				started = true;
			}

			write(1,LITLEN("<li><a href=\""));
			write(1,loc.s,loc.l);
			write(1,LITLEN("/"));

			char destname[0x100] = "index.html";
			int amt = LITSIZ("index.html");
			if(chapnum > 1) {
				amt = snprintf(destname,0x100, "chapter%d.html",chapnum);
				assert(amt < 0x100);
			}
			write(1,destname,amt);
			write(1,LITLEN("\">"));
			write(1,loc.s,loc.l);
			write(1,LITLEN(" "));
			write(1,destname,snprintf(destname, 0x100, "%d", chapnum));
			write(1,LITLEN("</a></li>\n"));
		}
		bool ret = git_for_chapters_changed(last,cur,on_chapter);
		if(started)
			write(1,LITLEN("</ul>\n</li>"));
	}

	git_for_commits(git_object_id(until)->id,NULL,on_commit);

	write(1,LITLEN("</ul></body></html>"));
	
	return 0;
}
