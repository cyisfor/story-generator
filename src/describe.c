#include "storydb.h"
#include "ensure.h"

#include <sys/mman.h>
#include <sys/wait.h> // waitpid
#include <sys/stat.h>

#include <fcntl.h> // O_*, open
#include <stdlib.h> // mkstemp
#include <readline/readline.h>
#include <unistd.h> // execlp, write

static
void for_story(void* udata,
							 identifier story,
							 const string title,
							 const string description,
							 const string source) {
	char tname[] = "tmpdescriptionXXXXXX";

	int t = mkstemp(tname);
			
	ensure_eq(description.len, write(t,description.base,description.len));

	if(title.base) {
		puts("****");
		puts(title.base);
		puts("****");
	} else {
		puts("No title");
	}
		
	int pid = fork();
	if(pid == 0) {
		execlp("emacsclient","emacsclient",tname,NULL);
	}
	waitpid(pid,NULL,0);
	// emacs likes to rename files into place xp

	close(t);
	t = open(tname,O_RDONLY);
	ensure_ge(t,0);
	struct stat info;
	fstat(t,&info);
	bstring newdesc = {
		mmap(NULL,info.st_size,PROT_READ,MAP_PRIVATE,t,0),
		info.st_size
	};
	ensure_ne(newdesc.base,MAP_FAILED);
	if(info.st_size == description.len && 0==memcmp(newdesc.base,description.base,description.len)) {
		puts("description unchanged");
	}

	if(title.base) {
		rl_insert_text(title.base);
	}
	string newtit = {
		.base = readline("Title: ")
	};
	newtit.len = strlen(newtit.base);
			
	string newsauce = {};
	if(source.len == 0) {
		newsauce.base = readline("Source: ");
		newsauce.len = strlen(newsauce.base);
	}
	storydb_set_info(story,newtit,CSTRING(newdesc),newsauce);
	munmap(newdesc.base,newdesc.len);
}

static
void have_info(void* udata,
							 const string title,
							 const string description,
							 const string source) {
	for_story(NULL, (intptr_t)udata, title, description, source);
}

int main(int argc, char *argv[])
{
	storydb_open();

	
	if(argc == 1) {
		storydb_for_undescribed(NULL, for_story);
	} else {
		int i;
		for(i=1;i<argc;++i) {
			string location = { argv[i], strlen(argv[i]) };
			identifier story = storydb_find_story(location);
			storydb_with_info((void*)story, have_info, story);
		}
	}
	db_close_and_exit();
	return 0;
}
