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
			
	ensure_eq(description.l, write(t,description.s,description.l));

	puts("****");
	puts(title.s);
	puts("****");
		
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
	mstring newdesc = {
		mmap(NULL,info.st_size,PROT_READ,MAP_PRIVATE,t,0),
		info.st_size
	};
	ensure_ne(newdesc.s,MAP_FAILED);
	if(info.st_size == description.l && 0==memcmp(newdesc.s,description.s,description.l)) {
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
	storydb_set_info(story,newtit,CSTR(newdesc),newsauce);
	munmap(newdesc.s,newdesc.l);
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
