static import makers;

import std.stdio: File;
import std.file: rename;
import std.process: spawnProcess, wait;

const string exe = "/home/code/htmlish/parse";

private class Maker : makers.Maker {
  override
  void make(string src, string dest) {
	File source = File(src,"r");
	File destination = File(dest~".temp","w");
	assert(0==
		   spawnProcess(exe,
						source,
						destination).wait());
	rename(dest~".temp",dest);
  }
}

Maker check;
