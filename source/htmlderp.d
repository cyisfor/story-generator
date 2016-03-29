static import html;

auto ref querySelector(html.Document* doc, html.HTMLString s) {
	import print: print;
	print("document should be",doc.root.document_);
  auto res = doc.querySelectorAll(s);
  if(res.empty) {
	import print: print;
	print("failed to find",s,doc.root.outerHTML);
  }
  print("result is",res.front.document_);
  assert(!res.empty);
  return res.front;
}

deprecated
auto createDocument(html.HTMLString source) {
  // no verbose entities in my utf-8 please
  return html.createDocument(source);
}

E detach(E)(E e) {
  e.detach();
  return e;
}

deprecated
auto htmlderp(NodeType)(NodeType n, html.HTMLString source) {
  return n.html(source);
}

unittest {
  import print: print;
  auto doc = createDocument("It's");
  print(doc.root.html);
  doc.root.html("It's");
  print(doc.root.html);
}
