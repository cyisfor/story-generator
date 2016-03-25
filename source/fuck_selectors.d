import html: HTMLString;
import cross.iter: filter;

auto for_all(bool depth_first = false,T)(T parent) {
  import std.range: chain;
  import cross.iter: map, joiner, filter;
  static if(depth_first) {
	return chain(map!for_all(parent.children).joiner,[parent]);
  } else {
	return chain([parent],parent.descendants);
  }
}

auto maybe4all(T)(T mayberange) {
  import cross.range: isInputRange;
  static if( isInputRange!T ) {
	return mayberange;
  } else {
	return for_all(mayberange);
  }
}

auto is_element(T)(T range) {
  return maybe4all(range).filter!((e) => e.isElementNode);
}

bool derp(HTMLString name,E)(E e) {
  return e.tag == name;
}

auto by_name(HTMLString name,T)(T range) {
  return maybe4all(range).is_element.filter!(derp!(name,T));
}
auto by_id(HTMLString ident,T)(T range) {
  return maybe4all(range).is_element.filter!((e) => e.attr("id") == ident);
}
auto by_class(HTMLString ident,T)(T range) {
  return maybe4all(range).is_element.filter!((e) => e.attr("class") == ident);
}
auto has_attr(HTMLString attr,T)(T range) {
  return maybe4all(range).is_element.filter!((e) => e.attr(attr) !is null);
}

unittest {
  import print: print;
  import std.array: array;
  import cross.iter: map;
  import html: createDocument;
  assert(["a","a","a"] ==
		 array(createDocument("<a/><a/><a/>")
			   .root.by_name!"a".map!"a.tag"));
  assert(["a","a","a"] ==
		 array(createDocument("<a/><a/><b/><a/>")
			   .root.by_name!"a".map!"a.tag"));
  assert(["a","a","a"] ==
		 array(createDocument(`<a id="1"/><a id="2"/><a id="1"/><a id="1"/>`)
			   .root.by_id!"1".map!"a.tag"));
  assert(["a","a","a"] ==
		 array(createDocument(`<a id="1"/> text in here
<a id="2">and here</a><a id="1"/><a id="1"/>`)
			   .root.by_id!"1".map!"a.tag"));
  assert(["a","a","a"] ==
		 array(createDocument(`<a foo="1"/><a/><a foo="1"/><a foo="1"/>`)
			   .root.has_attr!"foo".map!"a.tag"));
  assert(["a","a","a"] ==
		 array(createDocument(`<a class="1"/><a class="3"/><a class="1"/><a class="1"/>`)
			   .root.by_class!"1".map!"a.tag"));
  assert(createDocument(`<!DOCTYPE html> <html> <head><meta charset="utf-8">
  <title></title><header></header></head> <body><h1><intitle></intitle></h1>
  <top></top><content></content><footer></footer> </body></html`)
		 .root.by_name!"content".empty == false);
  
}  
