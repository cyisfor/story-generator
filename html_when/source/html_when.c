#include "html_when.h"
#include "selectors.h"
#include <string.h> // strlen
#include <stdlib.h> // getenv

// gumbo sucks
void gumbo_destroy_node(const GumboOptions* options, GumboNode* node);

bool is_element(GumboNode* n, const char* name, size_t nlen) {
	if(n->type != GUMBO_NODE_ELEMENT) return false;
	// gumbo suuuucks
	if(n->v.element.tag != GUMBO_TAG_UNKNOWN) return false;
	if(n->v.element.original_tag.length != nlen) return false;
	if(0!=strncasecmp(n->v.element.original_tag.data,name,nlen)) return false;
	return true;
}
void html_when(GumboNode* root) {
	if(!root) return;

	struct Selector selector = {};
	find_start(&selector, root, GUMBO_TAG_WHEN, NULL);
	for(;;) {
		GumboNode* cur = find_next(&selector);
		if(!cur) return;

		bool condition = true;
		int i;
		for(i=0;i<cur->v.element.attributes.length;++i) {
			GumboAttribute* a = (GumboAttribute*)cur->v.element.attributes.data[i];
			if(0==strcasecmp(a->name,"not")) {
				condition = !condition;
			} else if(a->name) {
				const char* name = a->name;
				while(*name && name[0] == '!') {
					++name;
					condition = !condition;
				}
				if(*name) {
					const char* match = getenv(name);
					if(!*a->value) {
						// without a value, no limitation to making falsity true and vice versa
						condition = !condition;
					} else {
						// a->value is already unescaped
						if(match && 0==strcasecmp(a->value,match)) {
							condition = !condition;
						}
					}
				}
			}
		}
		GumboVector* kids = &cur->v.element.children;
		int elsepoint = -1;
		if(condition) {
			// positive when clause
			// find else, if we can
			for(i=0;i<kids->length;++i) {
				GumboNode* kid = (GumboNode*) kids->data[i];
				if(kid->type == GUMBO_NODE_ELEMENT &&
					 kid->v.element.tag == GUMBO_TAG_ELSE) {
					// remove this, and all the rest after.
					elsepoint = i;
				}
				if(elsepoint >= 0) {
					gumbo_destroy_node(&kGumboDefaultOptions, kid);
				}
			}
			if(elsepoint >= 0) {
				kids->length = elsepoint;
			}
		} else {
			// negative when clause
			// try to find else...
			for(i=0;i<cur->v.element.children.length;++i) {
				GumboNode* kid = (GumboNode*) cur->v.element.children.data[i];
				if(kid->type == GUMBO_NODE_ELEMENT &&
					 kid->v.element.tag == GUMBO_TAG_ELSE) {
					elsepoint = i;
					gumbo_destroy_node(&kGumboDefaultOptions, kid);
					break;
				}
				gumbo_destroy_node(&kGumboDefaultOptions, kid);				
			}
			// move everything past elsepoint down to 0
			if(elsepoint == -1) {
				cur->v.element.children.length = 0;
			} else {
				int newlen = kids->length - elsepoint - 1;
				memmove(kids->data,
								kids->data + elsepoint + 1,
								sizeof(*kids->data) * newlen);
				kids->length = newlen;
			}
		}
	}
}
