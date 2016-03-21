import arsd.dom: Document, Element;

auto createElement(Document doc, string name, Element parent) {
  auto ret = doc.createElement(name);
  parent.appendChild(ret);
  return ret;
}

auto querySelector(Document doc, string s) {
  auto res = doc.querySelector(s);
  assert(res !is null);
  return res;
}
