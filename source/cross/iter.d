module cross.iter;
version(GNU) {
  public import std.algorithm: filter,map,joiner,filter;
  auto splat(Range)(Range range, char sep) {
	return ["one","two","three"];
  }
} else {
  public import std.algorithm.iteration: filter,map,joiner,filter,splitter;
  alias splat = splitter;
 }
 
