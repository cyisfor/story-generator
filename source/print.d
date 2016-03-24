string formatString(A...)(lazy A args) {
  import std.array: appender;
  import std.conv: to;
  import std.string: strip;
  
  auto app = appender!string();
  bool first = true;
  foreach(arg; args) {
	if(first)
	  first = false;
	else		
	  app.put(' ');	
	app.put(to!string(arg).strip());
  }
  return app.data;
}

version(printloc) {
  
void print(int line = __LINE__, string moduleName = __MODULE__, A...)(lazy A args) {
  import std.stdio;
  import std.conv: to;
  string prefix = moduleName ~ ":" ~ to!string(line) ~ " ";
  writeln(prefix ~ formatString!A(args));
}
} else {
  void print(A...)(lazy A args) {
	import std.stdio;
	writeln(formatString!A(args));
  }
 }
unittest {
  print(1,2,3,4);
}
