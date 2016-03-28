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

auto createDocument(html.HTMLString source) {
  // no verbose entities in my utf-8 please
  return html.createDocument!(html.DOMCreateOptions.None)(source);
}

E detach(E)(E e) {
  e.detach();
  return e;
}
