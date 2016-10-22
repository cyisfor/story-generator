import print: print;

import std.regex : regex, matchFirst;
import std.conv : to;
import std.process : pipeProcess, Redirect, wait;
import std.datetime : SysTime;
import std.functional : toDelegate;
import core.time: TimeException;

auto adp = regex("([^\t]+)\t([^\t]+)\t(.*)");

string parse_log(string since, void function(SysTime, string) handler) {
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

	import std.algorithm.searching: findSplit;
  foreach (line; git.stdout.byLine) {
		auto res = matchFirst(line,adp);
		if( res.empty ) {
			auto res2 = line.findSplit("\1");
			if( !res2 ) { continue; }
			try {
				modified = SysTime.fromISOExtString(res2[0]);
			} catch(TimeException e) {
				continue;
			}
			if(last_hash == null) {
				import std.conv: to;
				last_hash = to!string(res2[2]);
			} 
			continue;
		}
		// res[1], res[2]
		// can probably ignore whether it's adding or deleting
		// regenerate anyway
		handler(modified, to!string(res[3]));
  }
	print("hash is now",last_hash);
	return last_hash;
}

unittest {
  void handle(SysTime modified, string path) {
		print("hookay",modified,path);
  }

  parse_log("HEAD~2..HEAD",&handle);
}
