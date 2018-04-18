#include "string.h"
#include "storygit.h" // identifier
void create_setup(void);
int create_contents(identifier story,
										const string location,
										int dest,
										size_t chapters,
										void (*with_title)(identifier chapter,
																			 void(*handle)(const string title)));
void create_chapter(int src, int dest,
										int chapter, int chapters,
										identifier story, bool* title_changed);

#define CHAPTER_NAME(i,buf)											\
	char buf[0x100] = "index.html";								\
	int buf ## len = 10;													\
	if(i > 1) {																		\
		buf ## len = snprintf(buf,0x100,"chapter%ld.html",i);	\
	}

#define CHAPTER_NAME_STRING(i,name,buf)										\
	char buf[0x100] = "index.html";													\
	(name).s = buf;																					\
	if(i > 1) {																							\
		(name).l = snprintf(buf,0x100,"chapter%ld.html",i);		\
	} else {																								\
		(name).l = 10;																				\
	}
