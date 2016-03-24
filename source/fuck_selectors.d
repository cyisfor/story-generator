auto for_all!(bool depth_first = true)(auto parent) {
  import std.range: chain;
  import std.algorithm.iteration: map, joiner, filter;
  if(depth_first) {
	return chain(map!for_all(parent.children).joiner,[parent]);
  } else {
	return chain([parent],map!for_all(parent.children).joiner);
  }
}
auto is_element(auto range) {
  return range.filter!((e) => e.isElementNode);
}
auto by_name(HTMLstring name)(auto range) {
  return range.filter!((e) => e.name == name);
}
auto by_id(HTMLstring ident)(auto range) {
  return range.filter!((e) => e.attr("id") == ident);
}
auto by_class(HTMLstring ident)(auto range) {
  return range.filter!((e) => e.attr("class") == ident);
}
auto has_attr(HTMLstring attr)(auto range) {
  return range.filter!((e) => e.attr(attr) !is null);
}
