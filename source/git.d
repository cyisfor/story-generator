import std.regex : regex, matchFirst;
import std.conv : to;
import std.process : pipeProcess, Redirect, wait;
import std.stdio : writeln;
import std.datetime : SysTime;
import core.time: TimeException;

auto adp = regex("([^\t]+)\t([^\t]+)\t(.*)");

void parse_log(string since, void function(SysTime, string) handler) {
  string[] args;
  if(since == null) {
	args = ["git","log","--numstat","--pretty=format:%cI"];
  } else {
	args = ["git","log",since,"--numstat","--pretty=format:%cI"];
  }
  auto git = pipeProcess(args,Redirect.stdout);
  // why not an iterator? this:
  scope(exit) {
	git.stdout.close();
	git.pid.wait();
  }

  SysTime modified = 0;

  foreach (line; git.stdout.byLine) {
	//writeln(line);
	auto res = matchFirst(line,adp);
	if( res.empty ) {
	  try {
		modified = SysTime.fromISOExtString(line);
	  } catch(TimeException e) {}
	  continue;
	}
	// res[1], res[2]
	// can probably ignore whether it's adding or deleting
	// regenerate anyway
	handler(modified, to!string(res[3]));
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
