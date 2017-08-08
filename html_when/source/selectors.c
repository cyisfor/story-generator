#include "selectors.h"
#include <stdlib.h> // malloc...
#include <error.h>
enum directions { UP, DOWN, /*LEFT,*/ RIGHT} ;
/* Theseus strategy: if last direction was UP, check RIGHT, UP, then done
	 if last direction was RIGHT, check DOWN, RIGHT, UP
	 if last direction was DOWN, check DOWN, RIGHT, UP
	 keep right hand on the wall!
*/

static void find_destroy(struct Selector* s) {
	free(s->data);
	s->data = NULL; // just in case
	s->n = 0;
}

void find_start(struct Selector* s, GumboNode* top, Checker check, void* udata) {
	if(check) s->check = check;
	if(udata) s->udata = udata;
	if(top) s->cur = top;
	s->n = 0;
}

GumboNode* find_next(struct Selector* pos) {
#define POS pos->data[pos->n-1]
	GumboNode* cur = pos->cur;
	GumboNode* parent = cur->parent;
	bool right(void) {
		if(POS + 1 == cur->v.element.children.length) return false;
		cur = parent->v.element.children.data[++POS];
		return true;
	}
	bool down(void) {
		if(cur->v.element.children.length == 0) return false;
		if(pos->space <= pos->n) {
			pos->space += 0x400;
			pos->data = realloc(pos->data, pos->space);
		}
		pos->data[++pos->n] = 0; // always start on the left
		parent = cur;
		cur = cur->v.element.children.data[0];
		return true;
	}

	bool up(void) {
		if(!parent) return false;
		--pos->n;
		cur = parent;
		parent = parent->parent;
		return true;
	}
		
	enum directions d = DOWN; // "first" last move is down

	for(;;) {
		if(cur->type != GUMBO_NODE_ELEMENT) {
			if(pos->check(cur,pos->udata)) {
				pos->cur = cur;
				return cur;
			}
			// we can only go up
			if(!up()) {
				find_destroy(pos);
				return NULL;
			}
		}
		if(pos->check(cur,pos->udata)) {
			pos->cur = cur;
			return cur;
		}
		switch(d) {
		case UP:
			if(right()) d = RIGHT;
			else if(up()) d = UP;
			else {
				// we're done
				find_destroy(pos);
				return NULL;
			}
		case DOWN:
		case RIGHT:
			if(down()) d = DOWN;
			else if(right()) d = RIGHT;
			else if(up()) d = UP;
			else error(23,0,"couldn't move??");
		};
	}
}
