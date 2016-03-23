import std.stdio: File,writeln;
import std.file: rename;
import std.process: spawnProcess, wait, kill, Pid;
import std.socket: Socket, UnixAddress,
  AddressFamily, SocketType, ProtocolType;

immutable string exe = "code/htmlish/parse";
immutable string address = "parse.sock";

Socket socket;

class Watcher {
  Pid pid;
  
  this() {
	pid = spawnProcess([exe,address]);
  }
  auto connect() {
	socket = new Socket(AddressFamily.UNIX,
						SocketType.STREAM,
						ProtocolType.RAW);
	socket.connect(new UnixAddress(address));	
	return socket;
  }	  
  ~this() {
	pid.kill();
	pid.wait();
  }
}

void copy(File source, Socket dest) {
  static void[0x1000] buf;
  scope(exit) {
	source.close();
  }
  for(;;) {
	auto chunk = source.rawRead(buf);
	if( chunk.length == 0 ) break;
	dest.send(chunk);
  }
}

void copy(Socket source, File dest) {
  static void[0x1000] buf;
  scope(exit) {
	// om nom nom
	dest.close();
  }
  for(;;) {
	auto amt = source.receive(buf);
	if( amt == 0 ) break;
	dest.rawWrite(buf[0..amt]);
  }
}

Watcher watcher;
void make(string src, string dest) {
	writeln("htmlishhhhh ",dest);
	File source = File(src,"r");
	File destination = File(dest~".temp","w");
	auto socket = watcher.connect();
	copy(source,socket);
	copy(socket,destination);
	rename(dest~".temp",dest);
}
