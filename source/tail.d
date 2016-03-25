/* divide an input range in half, iterating up to n for .head, and past n
   for .tail
   .tail should not be iterated until .head is done.
   doesn't require ForwardRange!
*/

struct Divided(Range) {

  struct Head(Range) {
	size_t stopping_point;
	size_t progress;
	bool stopped;
	Range range;
	this(Range range, size_t stopping_point) {
	  this.range = range;
	  this.stopped = range.empty;
	  this.stopping_point = stopping_point;
	  this.progress = 0;
	}
	ElementType!Range front() {
	  return range.front;
	}
	bool empty() {
	  return stopped;
	}
	void popFront() {
	  if(stopped) return;
	  ++progress;
	  if(progress >= stopping_point) {
		stopped = true;
		return;
	  }
	  range.popFront();
	  stopped = range.empty;
	}
  }

  Head!Range head;
  Range tail;
};

Divided!Range divide(Range)(Range range, size_t n)
if(isInputRange!Range && !isInfinite!Range)
  {
	
	return Divided(Head(range),range);
  }

/* iterates up to n before the end for .head, and past that for .tail
   .tail has to be cached if it's not a ForwardRange, or hasLength, so
   beware of memory usage for large n!
   */

struct DivideEnd(Range) {
  static if( isForwardRange!Range || hasLength!Range ) {
	import std.range: tail;
	import std.traits: ReturnType;
	ReturnType!(tail!Range) tail;
	
  struct Head {
	size_t remaining;
	Range range;
	ElementType!Range[] ring;
	
  
