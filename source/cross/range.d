module cross.range;
version(GNU) {
  import std.range: isInputRange;
} else {
  import std.range.primitives: isInputRange;
 }
