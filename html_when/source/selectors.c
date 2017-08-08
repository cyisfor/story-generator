GumboNode* next_by_name(GumboNode* cur, const char* name, size_t nlen) {
	if(cur->type != GUMBO_NODE_ELEMENT) return NULL;
	
	size_t otherlen = strlen(cur->name);
	if(otherlen == nlen && 0==memcmp(cur->name,name,nlen)) {
		return cur;
	}
	int i;
	for(i=0;i<cur->v.element.children.length;++i) {
		GumboNode* next = next_by_name(cur->v.element.children.data[i],name,nlen);
		if(next) return next;
	}
	return NULL;
}
