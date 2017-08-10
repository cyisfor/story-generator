#include "string.h"
#include "storygit.h" // identifier
void create_setup(void);
int create_contents(const string location,
										const string dest,
										size_t chapters,
										void (*with_title)(identifier chapter,
																			 void(*handle)(const string title)));
void create_chapter(string src, string dest, int chapter, int chapters);
