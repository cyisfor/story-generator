import std.conv: emplace;
import std.array: Appender;
// sigh
template emplace_put(APP, Args...)
  if(is(APP == Appender!ArrayType))
{
  alias ElementType = ElementEncodingType!ArrayType;

  import std.conv: emplaceRef;
  void emplace_put(APP self, Args args) @trusted {
	self.ensureAddable(1);
	immutable len = self._data.arr.length;
	auto bigData = self._data.ptr[0..len + 1];
	emplaceRef!(Unqual!ElementType)(bigData[len], args);
	self._data.arr = bigData;
  }
}
				 
unittest {
  struct Uncopyable {
	@disable this(this);
	int foo;
  }

  auto a = appender!Uncopyable();
  a.emplace_put(1);
  a.emplace_put(2);
  a.emplace_put(3);
  import std.stdio;
  writeln(a.data);
  assert(a.data == [Uncopyable(1),Uncopyable(2),Uncopyable(3)]);
}
