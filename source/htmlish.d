static import makers;

import std.stdio: File,writeln;
import std.file: rename;
import std.process: spawnProcess, wait;

const string exe = "/home/code/htmlish/parse";

private class Maker : makers.Maker {
  override
  void make(string src, string dest) const {
	writeln("htmlishhhhh ",dest);
	File source = File(src,"r");
	File destination = File(dest~".temp","w");
	static import std.stdio;
	assert(0==
		   spawnProcess(exe,
						source,
						destination,
						std.stdio.stderr,
						["template": "template/chapter.xhtml"]).wait());
	rename(dest~".temp",dest);
	source.close();
	destination.close();
  }
}

const Maker check = new Maker();
