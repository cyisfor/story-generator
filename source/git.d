import print: print;

import std.regex : regex, matchFirst;
import std.conv : to;
import std.process : pipeProcess, Redirect, wait;
import std.datetime : SysTime;
import std.functional : toDelegate;
import core.time: TimeException;

auto adp = regex("([^\t]+)\t([^\t]+)\t(.*)");

void parse_log(string since, void function(SysTime, string) handler) {
 return parse_log(since,toDelegate(handler));
}

string parse_log(string since, void delegate(SysTime, string) handler) {
  string[] args;
  if(since == null) {
		args = ["git","log","--numstat","--pretty=format:%cI\1%H"];
  } else {
		args = ["git","log",since,"--numstat","--pretty=format:%cI\1%H"];
  }
  auto git = pipeProcess(args,Redirect.stdout);
  // why not an iterator? this:
  scope(exit) {
	git.stdout.close();
	git.pid.wait();
  }

  SysTime modified = 0;
	string last_hash = null;

  foreach (line; git.stdout.byLine) {
		auto res = matchFirst(line,adp);
		if( res.empty ) {
			res = line.findSplit("\1");
			if( res.empty ) { continue; }
			try {
				modified = SysTime.fromISOExtString(res[0]);
			} catch(TimeException e) {
				continue;
			}
			last_hash = res[2];
			continue;
		}
		// res[1], res[2]
		// can probably ignore whether it's adding or deleting
		// regenerate anyway
		handler(modified, to!string(res[3]));
  }
	return last_hash;
}

unittest {
  void handle(SysTime modified, string path) {
	print("hookay",modified,path);
  }

  parse_log("HEAD~2..HEAD",&handle);
}
