module cross.searching;
version(GNU) {
  public import std.algorithm: until,any;
} else {
  public import std.algorithm.searching: until,any;
 }
 
