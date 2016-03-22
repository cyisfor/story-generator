import std.container: RedBlackTree;
import std.functional: binaryFun;
import std.traits: hasMember;
import std.range: InputRange;
import std.algorithm.iteration: map;
import std.conv: to;

struct pair(K,V) {
  K key;
  V value;
  // for map()
  K key_of() {
	return key;
  }
  V value_of() {
	return value;
  }
  string toString() {
	return "(" ~ to!string(key) ~ "," ~ to!string(value) ~ ")"
}

bool key_less(T, alias less)(T a, T b)
  if( hasMember!(T,"key") )
	{
	  
	  return less(a.key, b.key);
	}
	  
template ordered_map(K, V, alias less = "a < b", bool allowDuplicates = false) {
  alias fless = binaryFun!less;
  alias entry = pair!(K, V);
  class ordered_map {
	
	RedBlackTree!(entry,
				  key_less!(entry,fless),
				  allowDuplicates) fuckyoufinal;
	InputRange!K keys() {
	  return map!(entry.key_of)(fuckyoufinal);
	}
	InputRange!V values() {
	  return map!(entry.value_of)(fuckyoufinal);
	}
	string toString() {
	  return fuckyoufinal.toString();
	}
  }
}
	
unittest {
  ordered_map!(string,int) foo;
  foo["z"] = 1;
  foo["y"] = 2;
  foo["x"] = 3;
  writeln(foo.keys,foo);
}
