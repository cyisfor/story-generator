import html: Document;

auto querySelector(Document doc, string s) {
  auto res = doc.querySelector(s);
  assert(res !is null);
  return res;
}
