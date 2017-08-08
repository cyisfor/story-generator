#include "gumbo.h"

// for moving up, has to keep a stack of cpos's...
// can't use call stack because next gets re-called... function pointers?
//typedef bool (*Checker)(GumboNode*,void*);
struct Selector {
	int* data; // make sure to init = {}
	size_t n;
	size_t space;
	void* udata;
	GumboTag check;
	GumboNode* cur;
};

void find_start(struct Selector* s, GumboNode* top, GumboTag check, void* udata);
GumboNode* find_next(struct Selector* pos);

