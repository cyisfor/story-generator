#include "db.h"
#include "ensure.h"
#include <sys/mman.h>
#include <stdlib.h> // mkstemp
#include <readline/readline.h>


int main(int argc, char *argv[])
{
	ensure_eq(argc,2);
	const char* dbs = "generate.sqlite";
	if(NULL != getenv("db")) dbs = getenv("db");
	db_open(dbs);

	string location = { argv[1], strlen(argv[1]) };
	identifier story = db_find_story(location);
	char tname[] = "tmpdescriptionXXXXXX";
	void have_info(const string title,
								 const string description,
								 const string source) {
		int t = mkstemp(tname);
			
		write(t,desc.s,desc.l);
		int pid = fork();
		if(pid == 0) {
			execlp("emacsclient","emacsclient",tname,NULL);
		}
		waitpid(pid,NULL,0);
		
		struct stat info;
		fstat(t,&info);
		mstring newdesc = {
			mmap(NULL,info.st_size,PROT_READ,MAP_PRIVATE,0,0),
			info.st_size
		};
		if(info.st_size == desc.l && 0==memcmp(newdesc.s,desc.s,desc.l)) {
			puts("description unchanged");
		}
		rl_insert_text(title.s);
		string newtit = {
			.s = readline("Title: ")
		};
		newtit.l = strlen(newtit.s);
			
		string newsauce = {};
		if(source.l == 0) {
			newsauce.s = readline("Source: ");
			newsauce.l = strlen(newsauce.s);
		}
		db_set_story_info(story,newtit,CSTR(newdesc),newsauce);
		munmap(newdesc.s,newdesc.l);
	}
	db_with_story_info(story, have_info);
	db_close_and_exit();
	return 0;
}
