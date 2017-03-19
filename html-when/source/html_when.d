import fuck_selectors: by_name;

import html.dom;

import std.array: array;

alias NodeType = typeof(Document.root);

void process_when(ref NodeType root) {
	foreach(ref e;root.by_name!"when".array) {
		import print: print;
		import std.process: environment;
		auto a = e.attrs().keys;
		auto b = a[0];
		auto c = environment.get(b);
		if(!(c is null)) {
			void subst_vals(NodeType e) {
				if(e.tag == "val") {
					e.html(c);
					return;
				}
				foreach(arg; e.by_name!"val") {
					arg.html(c);
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
			while(e.firstChild && e.firstChild.tag != "else") {
				e.firstChild.destroy();
			}
			while(e.firstChild) {
				e.firstChild.insertBefore(e);
			}
		}
		e.destroy();
	}
}
