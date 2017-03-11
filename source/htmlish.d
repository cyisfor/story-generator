b  //assert(dest.root.document_ == dest,to!string(dest));

  auto head = dest.root.by_name!"head".front;

  // find where we're going to dump this htmlish
  auto derp = dest.root.by_name!(ident);
  typeof(derp.front) content;
  if(derp.empty) {
		auto dsux = chain(dest.root.by_id!(ident),
											dest.root.by_name!"body");

		if(dsux.empty) {
			import print: print;
			print("no content found!");
			content = dest.createElement("body");
			dest.root.appendChild(content);
		} else {
			content = dsux.front;
			print("content derp",content.outerHTML);

		}
  } else {
		// have to replace <content>
		auto derrp = derp.front;
    assert(derrp);
		content = dest.createElement("div");
		foreach(e;derrp.children) {
			content.appendChild(detach(e));
		}
		content.insertAfter(derrp);
    derrp.destroy();
  }
	auto temp = dest.createElement("div");
	temp.html(source);
	process_root(dest, temp, head, title);
	import std.array: array;
	foreach(child;temp.children.array) {
		child.detach();
		content.appendChild(child);
	}

	destroy(temp);
  //print("processing htmlish for",title);
  process_root(dest,content, head, title);
  auto titles = dest.querySelectorAll("title");
  if(titles.empty) {
    auto tite = dest.createElement("title");
    tite.html = title;
    head.appendChild(tite);
  } else {
    auto tite = titles.front;
    tite.html = title;
  }
  foreach(intit; dest.querySelectorAll("intitle")) {
    auto texttit = dest.createTextNode(title);
    texttit.insertBefore(intit);
    intit.detach();
  }
  if(replace) {
		while(content.firstChild) {
			auto c = content.firstChild;
			detach(c).insertBefore(content);
		}
		content.detach();
  }
	auto date = dest.createElement("div");
	date.attr("class","ddate");
	static import ddate;
	date.appendChild(dest.createTextNode("This page was created on "));
	date.appendChild(dest.createTextNode(to!string(ddate.current(modified))));
	dest.root.by_name!"body"
		.front.appendChild(date);
  //assert(dest.root.document_ == dest,to!string(dest));
  return move(dest);
}

int derp = 0;

long modified = -1;

void make(string src, string dest, Document templit = default_template) {
  import std.file: readText,write,rename;
  import htmlderp: querySelector;
  auto root = parse(readText(src),templit).root;

  // append to the file in place
  import std.stdio;
  struct Derpender {
		File ou;
		void put(T)(T s) {
			ou.write(to!string(s));
			ou.flush();
		}
  }
  File destf = File(dest~".temp","wt");
  scope(exit) destf.close();
  auto app = Derpender(destf);
  root.innerHTML(app);
  rename(dest~".temp",dest);
}

auto make(Document templit) {
	void derp(string src, string dest) {
		make(src,dest,templit);
	}
	return &derp;
}

unittest {
  import std.stdio: writeln,stdout;
  static import backtrace;
  static backtrace.PrintOptions opts = {
	colored:  true,
	detailedForN: 5,
  };
  backtrace.install(stdout,opts,5);
  auto p = parse(`hi
there
this
is <i>a</i> test`);
  writeln(p.root.html);
}

unittest {
  import std.stdio;
  auto p = parse(`herp
Italics should <i>not</i> be shoved to the back.
derp`);
  writeln(p.root.html);
}

unittest {
  import std.stdio;
  auto p = parse(`after <div hish="1"> a <u>tag</u>
With newlines, it should be a new paragraph.</div>

With only space at the <u>end</u>
It should still be a new paragraph.`);
  writeln(p.root.html);
}
