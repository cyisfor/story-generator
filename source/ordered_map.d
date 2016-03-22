import std.container: RedBlackTree;
import std.functional: binaryFun;
import std.traits: hasMember;
import std.range: InputRange, drop, take;
import std.algorithm.iteration: map;
import std.conv: to;

struct pair(K,V) {
  K key;
  V value;

  string toString() {
	return "(" ~ to!string(key) ~ "," ~ to!string(value) ~ ")";
  }
}

bool key_less(T, alias less)(T a, T b)
  if( hasMember!(T,"key") )
	{
	  
	  return less(a.key, b.key);
	}

template ordered_map(K, V, alias less = "a < b", bool allowDuplicates = false) {
  alias fless = binaryFun!less;
  alias entry = pair!(K, V);

  K key_of(entry self) {
	return self.key;
  }
  V value_of(entry self) {
	return self.value;
  }

  alias sub_tree = 	RedBlackTree!(entry,
								  key_less!(entry,fless),
								  allowDuplicates);

  class ordered_map {
	
	sub_tree fuckyoufinal;
	
	auto keys() {
	  return map!(key_of)(this[0..$]);
	}
	auto values() {
	  return map!(value_of)(this[0..$]);
	}
	override
	string toString() {
	  return to!string(fuckyoufinal);
	}
	@property
	size_t opDollar() {
	  return fuckyoufinal.length;
	}
	@property
	auto opSlice(size_t a, size_t b) {
	  return take(drop(fuckyoufinal[],a),b);
	}
	@property
	auto opSlice() {
	  return fuckyoufinal[];
	}
	@property
	auto opIndex(K key) {
	  return fuckyoufinal.equalRange(key)[0];
	}
  }
}
	
unittest {
  ordered_map!(string,int) foo;
  foo["z"] = 1;
  foo["y"] = 2;
  foo["x"] = 3;
  import std.stdio: writeln;
  writeln(foo.keys,foo);
}
