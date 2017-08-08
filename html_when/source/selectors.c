enum { UP, DOWN, /*LEFT,*/ RIGHT} directions;
/* Theseus strategy: if last direction was UP, check RIGHT, UP, then done
	 if last direction was RIGHT, check DOWN, RIGHT, UP
	 if last direction was DOWN, check DOWN, RIGHT, UP
	 keep left hand on the wall!
*/

// for moving up, has to keep a stack of cpos's...
// can't use stack because next gets re-called... function pointers?
struct cposes {
	int* data;
	size_t n;
	size_t space;
};

GumboNode* next_by_name(struct cposes* pos, GumboNode* cur, const char* name, size_t nlen) {
	#define POS pos->data[pos->n-1];
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
		cur = parent;
		parent = parent->parent;
		return true;
	}
		
	directions d = DOWN; // "first" last move is down

	for(;;) {
		if(cur->type != GUMBO_NODE_ELEMENT) return NULL;
		size_t otherlen = strlen(cur->name);
		if(otherlen == nlen && 0==memcmp(cur->name,name,nlen)) {
			return cur;
		}
		switch(d) {
		case UP:
			if(right()) d = RIGHT;
			else if(up()) d = UP;
			else return NULL; // couldn't find it...
		case DOWN:
		case RIGHT:
			if(down()) d = DOWN;
			else if(right()) d = RIGHT;
			else if(up()) d = UP;
			else error(23,0,"couldn't move??");
		};
	}
}
