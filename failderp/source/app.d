import htmlderp: createDocument, querySelector;
//import html: createDocument, DOMCreateOptions;
void main() {
  import std.stdio;
  auto contents = createDocument(`<p id="description"></p>`);
  const(char)[] description = `test<i>this</i>`;
  auto desc = contents.querySelector("#description");
  assert(desc);
  writeln("herp");
  desc.html(description);
  writeln(contents.root.html);
  
}

