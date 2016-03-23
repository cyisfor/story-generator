static import html;
import std.stdio: writeln;

auto querySelector(ref html.Document doc, html.HTMLString s) {
  auto res = doc.querySelector(s);
  if(res is null) {
	writeln("failed to find ",s);
	writeln(doc.root.outerHTML);
  }
  assert(res !is null);
  return res;
}

auto createDocument(html.HTMLString source) {
  return html.createDocument!(html.DOMCreateOptions.None)(source);
}
