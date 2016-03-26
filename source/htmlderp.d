static import html;

auto querySelector(ref html.Document doc, html.HTMLString s) {
	print("document should be",&doc);
  auto res = doc.querySelectorAll(s);
  if(res.empty) {
	import print: print;
	print("failed to find",s,doc.root.outerHTML);
  }
  assert(!res.empty);
  return res.front;
}

alias createDocument =
  html.createDocument!(html.DOMCreateOptions.None);

E detach(E)(E e) {
  e.detach();
  return e;
}
