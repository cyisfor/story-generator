import std.container.rbtree: RedBlackTree;
import std.container.util: make;
import std.functional: binaryFun;
import std.traits: hasMember;
import std.range: InputRange, drop, take;
import std.algorithm.iteration: map;
import std.conv: to;
import std.typecons: Nullable;

struct pair(K,V) {
  K key;
  Nullable!V value;
  this(K k, V v) {
	key = k;
	value = v;
  }
  string toString() {
	string vstr;
	if(value.isNull()) {
	  vstr = "(null)";
	} else {
	  vstr = to!string(value.get());
	}
		
	return "(" ~ to!string(key) ~ "," ~ vstr ~ ")";
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
	return self.value.get();
  }

  alias sub_tree = 	RedBlackTree!(entry,
								  key_less!(entry,fless),
								  allowDuplicates);

  class ordered_map {
	
	sub_tree fuckyoufinal;

	this() {
	  fuckyoufinal = make!(sub_tree)();
	}
	
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
	V opIndex(K key) {
	  Nullable!V nothin;
	  auto r = fuckyoufinal.equalRange(entry(key,nothin));
	  static if(allowDuplicates) {
		return map!(&value_of)(r);
	  } else {
		return r.front.value.get();
	  }
	}
	@property
	auto opIndexAssign(V value, K key) {
	  return fuckyoufinal.insert(entry(key,value));
	}
  }
}

version(unittestderp) { import unit_threaded; }
else              {
  void shouldEqual(T)(T a, T b) {
	assert(a==b);
  }
 } // so production builds compile

unittest {
  ordered_map!(string,int) foo = make!(ordered_map!(string,int));
  foo["z"] = 1;
  foo["y"] = 2;
  foo["x"] = 3;
  import std.stdio: writeln;
  foo.keys.front.shouldEqual("x");
  writeln("keys: ",foo.keys);
  writeln("both: ",foo);
}
