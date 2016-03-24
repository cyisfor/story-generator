bool printloc = false;
shared static this() {
  import core.stdc.stdlib: getenv;
  if(getenv("printloc"))
	printloc = true;
} 

string formatString(A...)(int line, string moduleName, lazy A args) {
  import std.array: appender;
  import std.conv: to;
  import std.string: strip;

  auto app = appender!string();
  if(printloc) {
	app.put(moduleName);
	app.put(':');
	app.put(to!string(line));
	app.put(' ');
  }

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


void print(int line = __LINE__, string moduleName = __MODULE__, A...)(lazy A args) {
  import std.stdio;
  import std.conv: to;
  writeln(formatString!A(line,moduleName,args));
}

unittest {
  print(1,2,3,4);
}
