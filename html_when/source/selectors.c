enum { UP, DOWN, /*LEFT,*/ RIGHT} directions;
/* Theseus strategy: if last direction was UP, check LEFT, UP, then done
	 if last direction was RIGHT, check DOWN, RIGHT, UP
	 if last direction was DOWN, check DOWN, RIGHT, UP
	 keep left hand on the wall!
*/

GumboNode* next_by_name(GumboNode* cur, const char* name, size_t nlen) {
	int cpos = 0;
	GumboNode* parent = cur->parent;
	bool left(void) {
		if(cpos == 0) return false;
		cur = parent->v.element.children.data[--cpos];
		return true;
	}
	bool right(void) {
		if(cpos + 1 == cur->v.element.children.length) return false;
		cur = parent->v.element.children.data[++cpos];
		return true;
	}
	bool up(void) {
		if(parent == NULL) return false;
		cur = parent;
		parent = parent->parent;
		return true;
	}
	bool down(void) {
		if(cur->v.element.children.length == 0) return false;
		cpos = 0;
		parent = cur;
		cur = cur->v.element.children.data[cpos];
		return true;
	}

		if(cpos + 1 == cur->v.element.children.length) return false;
	bool right(void)

	for(;;) {
		if(cur->type != GUMBO_NODE_ELEMENT) return NULL;
		
		size_t otherlen = strlen(cur->name);
		if(otherlen == nlen && 0==memcmp(cur->name,name,nlen)) {
			return cur;
		}
		switch(d) {
		case UP:

				// LEFT

			} else if(cpos + 1 < parent->v.element.children.length) {
				// RIGHT
				cur = parent->v.element.children.data[++cpos];
			} else {
				// UP again
				GumboNode* t = parent;
				cur = parent;
				parent = t->parent;
			}
			continue;
		case DOWN:
			if(cpos > 0) {
				// LEFT
				cur = parent->v.element.children.data[--cpos];
			} else if(cpos + 1 < parent->v.element.children.length) {
				// RIGHT
				cur = parent->v.element.children.data[++cpos];
			} else {
				// DOWN again
				GumboNode* t = parent;
				parent = cur;
				cur = cur->v.element.children.data[cpos];
			}
			continue;
		case LEFT:

			
					
		int i;
		for(i=0;i<cur->v.element.children.length;++i) {
			GumboNode* next = next_by_name(cur->v.element.children.data[i],name,nlen);
			if(next) return next;
	}
	return NULL;
}
