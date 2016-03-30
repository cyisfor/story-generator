static import html;
    import std.conv: to;

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
	  if(cur.toHash in seen) return;
	  pass1(cur);
	  cur = cur.nextSibling;
	}
  }
  pass1(root);
  writeln(repeated.length," repeated");
  seen.clear();
  void pass2(NodeType cur, size_t depth) {
	if(cur == null)
	  return;
	if(!cur.isElementNode) return;
	if(cur.toHash in seen) 
	  return;
	seen[cur.toHash] = true;
	write(replicate(" ",depth));
	if(auto num = cur.toHash in repeated) {
	  write('(',*num,')');
	}
	if(cur.previousSibling) {
	  if(cur.previousSibling.isTextNode)
		writef("(text %d:%x)",to!string(cur.previousSibling).length,cur.previousSibling.toHash());
	  else
		writef("%s:%x",cur.previousSibling.tag,cur.previousSibling.toHash);
	} else {
	  write("(null)");            
	}
	writef("-> %s:%x ->",cur.tag,cur.toHash);
	if(cur.nextSibling) {
	  if(cur.nextSibling.isTextNode)
		writefln("(text %d:%x)",to!string(cur.nextSibling).length,cur.nextSibling.toHash);
	  else
		writefln("%s:%x",cur.nextSibling.tag,cur.nextSibling.toHash);
	} else {
	  writeln("(null)");
	}
	cur = cur.firstChild;
	while(cur) {
	  if(cur.toHash in seen) return;
	  pass2(cur,depth+1);
	  cur = cur.nextSibling;
	}
  }
  pass2(root,0);
}


auto ref querySelector(html.Document* doc, html.HTMLString s) {
  import print: print;
  print("document should be",doc.root.document_);
  auto res = doc.querySelectorAll(s);
  if(res.empty) {
	import print: print;
	print("failed to find",s,doc.root.outerHTML);
    throw new Exception(to!string(s));
  }
  print("result is",res.front.document_);
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
