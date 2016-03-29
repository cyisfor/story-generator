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
