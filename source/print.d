bool printloc = false;
size_t margin = 2;

shared static this() {
	import core.stdc.stdlib: getenv;
	if(getenv("printloc"))
		printloc = true;
}

string formatString(A...)(int line, string moduleName, lazy A args) {
	import std.array: appender;
	import std.conv: to;
	import std.string: strip, wrap;
	import std.range: take, repeat;

	string space(ulong amount) {
		return take(repeat(' '),amount).to!string;
	}


	auto prefix = appender!string();
	if(printloc) {
		prefix.put(moduleName);
		prefix.put(':');
		prefix.put(to!string(line));
		prefix.put(' ');

		import std.algorithm: min;
		if(margin < prefix.data.length)
			margin = min(12,prefix.data.length);
		else if(margin > prefix.data.length) {
			prefix.put(space(margin-prefix.data.length));
		}
	}
	// can't just wrap after, because wrap() secretly collapses whitespace
	// the sneaky jerks!
	auto rest = appender!string;

	bool first = true;
	foreach(arg; args) {
		if(first)
			first = false;
		else		
			rest.put(' ');	
		rest.put(to!string(arg).strip());
	}

	return prefix.data ~ wrap(rest.data,80-margin,null,space(margin),4);
}


void print(int line = __LINE__, string moduleName = __MODULE__, A...)(lazy A args) {
	import std.stdio: write;
	write(formatString!A(line,moduleName,args));
}

unittest {
	print(1,2,3,4);
}
