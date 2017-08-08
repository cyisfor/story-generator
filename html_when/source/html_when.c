#include "selectors.h"

#include "gumbo.h"

void html_when(GumboOutput* root) {
	GumboNode* cur = root->document;
	while(cur) {
		cur = next_by_name(cur,"when");
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
				if(strcasecmp(kid->name,"else")) {
					// remove this, and all the rest after.
					elsepoint = i;
				}
				if(elsepoint >= 0) {
					gumbo_destroy_node(&kDefaultOptions, kid);
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
				if(strcasecmp(kid->name,"else")) {
					elsepoint = i;
					gumbo_destroy_node(&kDefaultOptions, kid);
					break;
				}
				gumbo_destroy_node(&kDefaultOptions, kid);				
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

void process_when(Document* doc) {
	auto root = doc.root;
	process_when(root);
}