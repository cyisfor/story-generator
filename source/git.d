import std.regex : regex, matchFirst;
import std.conv : to;
import std.process : pipeProcess, Redirect, wait;
import std.path : pathSplitter, stripExtension, extension;
import std.stdio : writeln;
import std.array : array;
import std.string : isNumeric, strip;
import std.datetime : SysTime;
version(GNU) {
  import std.algorithm: startsWith;
} else {
  import std.algorithm.searching: startsWith;
 }

auto adp = regex("([^\t]+)\t([^\t]+)\t(.*)");

void parse_log(string since, void function(SysTime, int, string, bool) handler) {
  string[] args;
  if(since == null) {
	args = ["git","log","--numstat","--pretty=format:%at"];
  } else {
	args = ["git","log",since,"--numstat","--pretty=format:%at"];
  }
  auto git = pipeProcess(args,Redirect.stdout);
  // why not an iterator? this:
  scope(exit) {
	git.stdout.close();
	git.pid.wait();
  }

  int modified = 0;

  foreach (line; git.stdout.byLine) {
	//writeln(line);
	auto res = matchFirst(line,adp);
	if( res.empty ) {
	  if(isNumeric(line)) {
		modified = to!(int)(line);
	  }
	  continue;
	}
	auto path = array(pathSplitter(res[3]));
	if(path.length != 3) continue;
	if(path[1] != "markup") continue;

	string name = stripExtension(to!string(path[path.length-1]));
	string ext = extension(to!string(path[path.length-1]));
	bool is_hish = ext == ".hish";
	if(!(ext == ".txt" || is_hish)) continue;

	if(!name.startsWith("chapter")) continue;
	
	string derp = name["chapter".length..name.length];
	if(!isNumeric(derp)) {
	  continue;
	}
	int chapter = to!int(derp);

	// can probably ignore whether it's adding or deleting
	// regenerate anyway
	handler(SysTime.fromUnixTime(modified), chapter, to!string(path[0]), is_hish);
  }
}

unittest {
  void handle(int modified, int chapter, string story, bool is_hish) {
	writeln("hokay " ~
			to!(string)(modified) ~ " " ~
			to!(string)(chapter) ~ " for " ~
			story);
  }
  parse_log("HEAD~2..HEAD",&handle);
}
