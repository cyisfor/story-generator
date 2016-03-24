auto for_all!(bool depth_first = false)(auto parent) {
  import std.range: chain;
  import std.algorithm.iteration: map, joiner, filter;
  static if(depth_first) {
	return chain(map!for_all(parent.children).joiner,[parent]);
  } else {
	return chain([parent],parent.descendants);
  }
}

auto maybe4all(auto mayberange) {
  import std.range.primitives: isInputRange;
  static if( isInputRange(mayberange) ) {
	return mayberange;
  } else {
	return for_all(maybe);
  }
}

auto is_element(auto range) {
  return maybe4all(range).filter!((e) => e.isElementNode);
}

auto by_name(HTMLstring name)(auto range) {
  return maybe4all(range).is_element.filter!((e) => e.name == name);
}
auto by_id(HTMLstring ident)(auto range) {
  return maybe4all(range).is_element.filter!((e) => e.attr("id") == ident);
}
auto by_class(HTMLstring ident)(auto range) {
  return maybe4all(range).is_element.filter!((e) => e.attr("class") == ident);
}
auto has_attr(HTMLstring attr)(auto range) {
  return maybe4all(range).is_element.filter!((e) => e.attr(attr) !is null);
}
