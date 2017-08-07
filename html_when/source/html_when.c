#include "fuck_selectors.h"

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
					if(match == NULL) {
						condition = !condition;
					} else {
						if(*a->value
						
		if(inverted) {
			gumbo_vector_remove(NULL, inverted, &cur->v.element.attributes);
		}
		auto a = e.attrs().keys;
//		print("DERP",inverted,a);
		auto b = a[0];
		if(b[0] == '!') {
			b = b[1..$].strip();
//			print("um",b);
			inverted = !inverted;
		}
		auto c = environment.get(b);
		auto match = e.attr(b);
//		print("umm",c,match);
		if(match != "") {
			match = unescape(match);
		}
		bool condition() {
			if(inverted) {
				if(match == "") {
					return c is null;
				} else {
					return c != match;
				}
			}
			if(match == "") {
				return c !is null;
			}
			return c == match;
		}
		if(condition()) {
			void subst_vals(NodeType e) {
				void replace_html(ref NodeType e) {
					e.html(c);
					while(e.firstChild) {
						e.firstChild.insertBefore(e);
					}
					e.destroy();
				}
				if(e.tag == "val") {
					replace_html(e);
					return;
				}
				foreach(arg; e.by_name!"val") {
					replace_html(arg);
					while(arg.firstChild) {
						arg.firstChild.insertBefore(arg);
					}
					arg.destroy();
				}
			}
			while(e.firstChild && e.firstChild.tag != "else") {
				subst_vals(e.firstChild);
				e.firstChild.insertBefore(e);
			}
		} else {
			while(e.firstChild) {
				if(e.firstChild.tag == "else") {
					e.firstChild.destroy();
					while(e.firstChild) {
						e.firstChild.insertBefore(e);
					}
				} else {
					e.firstChild.destroy();
				}
			}
		}
		e.destroy();
	}
}

void process_when(Document* doc) {
	auto root = doc.root;
	process_when(root);
}
