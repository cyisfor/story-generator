import html: Document;
import std.stdio: writeln;

auto querySelector(Document doc, string s) {
  auto res = doc.querySelector(s);
  if(res is null) {
	writeln(doc.root.outerHTML);
  }
  assert(res !is null);
  return res;
}
