static import html;
    import std.conv: to;

static if(false) {
void print_with_cycles(NodeType)(NodeType root) {
  import std.stdio;
  import std.array: replicate;
  import std.string: strip;
  bool[size_t] seen;
  uint[size_t] repeated;
  uint ident = 0;
  NodeType cur = root;
  void pass1(NodeType cur) {
	if(cur == null)
	  return;
	if(!cur.isElementNode) return;
	if(cur.toHash in seen) {
	  writeln("Cycle in ",cur.tag,"!");
	  repeated[cur.toHash] = ++ident;
	  return;
	}
	seen[cur.toHash] = true;
	cur = cur.firstChild;
	while(cur) {
	  if(cur.toHash in seen) {
		writeln("Cycle in ",cur.tag,"!");
		repeated[cur.toHash] = ++ident;
		return;
	  }
	  pass1(cur);
	  cur = cur.nextSibling;
	}
  }
  pass1(root);
  writeln(repeated.length," repeated");
  seen.clear();
  void writeathing(NodeType cur) {
	if(cur == null) {
	  write("(null)");
	  return;
	}
	if(auto num = cur.toHash in repeated) {
	  write('[',*num,']');
	}
	if(cur.isElementNode) {
	  writef("(%s:%x)",cur.tag,cur.toHash % 0x10000);
	} else {
		writef("(text %d:%x)",cur.text.length,
			   cur.toHash  % 0x10000);
	}
  }

  void pass2(NodeType cur, size_t depth) {
	if(cur == null)
	  return;
	if(!cur.isElementNode) return;
	if(cur.toHash in seen)
	  return;
	seen[cur.toHash] = true;
	write(replicate(" ",depth));
	writeathing(cur.previousSibling);
	write(" -> ");
	writeathing(cur);
	write(" -> ");
	writeathing(cur.nextSibling);
	writeln("");

	cur = cur.firstChild;
	while(cur) {
	  if(cur.toHash in seen) return;
	  pass2(cur,depth+1);
	  cur = cur.nextSibling;
	}
  }
  pass2(root,0);
}
}

auto ref querySelector(Document)(Document doc, html.HTMLString s) {
  auto res = doc.querySelectorAll(s);
  if(res.empty) {
	import print: print;
	print("failed to find",s,doc.root.outerHTML);
    throw new Exception(to!string(s));
  }
  assert(!res.empty);
  return res.front;
}

E detach(E)(E e) {
  e.detach();
  return e;
}

unittest {
  import print: print;
  auto doc = html.createDocument("It's");
  print(doc.root.html);
  doc.root.html("It's");
  print(doc.root.html);
}
