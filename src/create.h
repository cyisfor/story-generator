#include "string.h"
#include "storygit.h" // identifier
void create_setup(void);

int create_contents(identifier story,
					const string title_file,
					const string location,
					int dest,
					size_t chapters);

void create_chapter(int src, int dest,
					int chapter, int chapters,
					identifier story, bool* title_changed);

#define CHAPTER_NAME(i,buf)										\
	char buf[0x100] = "index.html";								\
	int buf ## len = 10;										\
	if(i > 1) {													\
		buf ## len = snprintf(buf,0x100,"chapter%ld.html",i);	\
	}

#define CHAPTER_NAME_STRING(i,name,buf)							\
	if(i > 1) {													\
		(name).base = buf;										\
		(name).len = snprintf(buf,0x100,"chapter%ld.html",i);	\
	} else {													\
		(name).base = "index.html";								\
		(name).len = 10;										\
	}
