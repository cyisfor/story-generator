import fuck_selectors: by_name;

import html.dom;

import std.array: array;

alias NodeType = typeof(Document.root);

void process_when(ref NodeType root) {
	foreach(ref e;root.by_name!"when".array) {
		import print: print;
		import std.process: environment;
		import std.string: strip, startsWith;
					
		bool inverted = e.attr("not") !is null;
		if(inverted) e.attrs.remove("not");
		auto a = e.attrs().keys;
		print("DERP",a);
		auto b = a[0];
		if(b[0] == '!') {
			b = b[1..$].strip();
			print("um",b);
			inverted = !inverted;
		} 
		auto c = environment.get(b);
		bool condition() {
			if(inverted)
				return c is null;
			return c !is null;
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
