import std.conv: emplace;
import std.array: Appender;
import std.range.primitives : ElementEncodingType;
// sigh
template emplace_put(APP: Appender!ArrayType, ArrayType, Args...)
{
  alias ElementType = ElementEncodingType!ArrayType;

  import std.conv: emplaceRef;

}
				 
