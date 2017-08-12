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
