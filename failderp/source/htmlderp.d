static import html;
import std.stdio: writeln;

auto querySelector(ref html.Document doc, html.HTMLString s) {
  auto res = doc.querySelectorAll(s);
  if(res.empty) {
	writeln("failed to find ",s);
	writeln(doc.root.outerHTML);
  }
  assert(!res.empty);
  return res.front;
}

auto createDocument(html.HTMLString source) {
  return html.createDocument!(html.DOMCreateOptions.None)(source);
}
