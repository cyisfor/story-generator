#include "output.h"

#include <unistd.h>
#include <string.h>
#include <error.h>

#define LITLEN(a) a,sizeof(a)-1
#define STRLEN(a) (a),strlen(a)

void output(int dest, GumboNode* cur) {
	switch(cur->type) {
	case GUMBO_NODE_TEXT:
	case GUMBO_NODE_CDATA:
	case GUMBO_NODE_WHITESPACE:
		write(dest,cur->v.text.text,strlen(cur->v.text.text));
		return;
	case GUMBO_NODE_COMMENT:
		write(dest,LITLEN("<!--"));
		write(dest,cur->v.text.text,strlen(cur->v.text.text));
		write(dest,LITLEN("-->"));
		return;
	case GUMBO_NODE_DOCUMENT:
		write(dest,LITLEN("<!DOCTYPE html>\n"));
		// fall through
	case GUMBO_NODE_ELEMENT:
		{ GumboElement* e = &cur->v.element;
			write(dest,LITLEN("<"));
			write(dest,e->original_tag.data,e->original_tag.length);
			int i;
			for(i=0;i<e->attributes.length;++i) {
				write(dest,LITLEN(" "));
				GumboAttribute* a = e->attributes.data[i];
				write(dest,STRLEN(a->name));
				if(0==a->original_value.length) {
					write(dest,LITLEN("="));
					// includes quotation marks
					write(dest,a->original_value.data,a->original_value.length);
				}
			}
			if(0 == e->children.length) {
				write(dest,LITLEN(" />"));
			} else {
				write(dest,LITLEN(">"));
				for(i=0;i<e->children.length;++i) {
					output(dest,e->children.data[i]);
				}
				write(dest,LITLEN("</"));
				write(dest,e->original_tag.data,e->original_tag.length);
				write(dest,LITLEN(">"));
			}
		}
	default:
		error(23,0,"bad node type... wat is template?");
	};
}
