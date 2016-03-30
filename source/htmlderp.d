static import html;

void print_with_cycles(NodeType)(NodeType root) {
    import std.stdio;
    import std.array: replicate;
    import std.string: strip;
    bool[NodeType] seen;
    uint[NodeType] repeated;
    uint ident = 0;
    NodeType cur = root;
    void pass1(NodeType cur) {
        if(cur == null)
            return;
        if(!cur.isElementNode) return;
        if(cur in seen) {
            writeln("Cycle in ",cur.tag,"!");
            repeated[cur] = ++ident;
            return;            
        }
        seen[cur] = true;
        cur = cur.firstChild;
        while(cur) {
            if(cur in seen) return;
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
        if(cur in seen) 
            return;
        seen[cur] = true;
        write(replicate(" ",depth));
        if(auto num = cur in repeated) {
            write('(',*num,')');
        }
        if(cur.previousSibling) {
            write(cur.previousSibling.tag);
        } else {
            write("(null)");            
        }
        write("->" ~ cur.tag ~ "->");
        if(cur.nextSibling) {
            writeln(cur.nextSibling.tag);
        } else {
            writeln("(null)");
        }
        cur = cur.firstChild;
        while(cur) {
            if(cur in seen) return;
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
    import std.conv: to;
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
