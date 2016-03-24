import htmlderp: createDocument, querySelector;
void main() {
  import std.stdio;
  auto contents = createDocument(`<p id="description"></p>`);
  const(char)[] description = `test`;
  auto desc = querySelector(contents,"#description");
  //contents.check();
  assert(!desc.empty);
  desc.front.html(description);
  writeln("derp");
//contents.check();
}

