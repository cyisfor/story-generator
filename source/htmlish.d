import print: print;
import htmlderp: createDocument, querySelector;
import html: Document, HTMLstring;

import std.stdio: File;
import std.file: rename;

Document default_template;
static this() {
  // one to be there at compile time j/i/c
  default_template = createDocument(import("template/default.html"));
}



void process_root(auto root) {

auto parse(HTMLstring source, ref Document templit = null) {
  if(templit == null) {
	templit = default_template;
  }
  bool ended_newline = false;
  auto dest = templit.clone();
  auto content = chain(dest.root.by_name("content"),
					   dest.root.by_name("div")
  

  fuck_selectors.has_attr(src.root,"hish").each(&process_root);
	
  
  class Watcher {
  Pid pid;
  bool disabled;
  void get() {
	if(pid !is null) return;
	disabled = false;
	try {
	  pid = spawnProcess([exe,address],
						 [
						  "template":"template/chapter.xhtml",
						  "title": "???"
						  ]);
	} catch(ProcessException e) {
	  disabled = true;
	}
  }
  auto connect() {
	get();
	assert(!disabled);
	socket = new Socket(AddressFamily.UNIX,
						SocketType.STREAM,
						ProtocolType.IP);
	print(address);
	for(;;) {
	  try {
		socket.connect(new UnixAddress(address));
		break;
	  } catch(SocketOSException e) {
	  }
	}
	return socket;
  }	  
  ~this() {
	if(pid !is null) {
	  pid.kill();
	  pid.wait();
	}
  }
}

void copy(File source, Socket dest) {
  static void[0x1000] buf = void;
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
  static void[0x1000] buf = void;
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
static this() {
  watcher = new Watcher();
}
void make(string src, string dest) {
	print("htmlishhhhh ",dest);
	File source = File(src,"r");
	File destination = File(dest~".temp","w");
	auto socket = watcher.connect();
	copy(source,socket);
	socket.shutdown(SocketShutdown.SEND);
	print("Okay, sent it");
	copy(socket,destination);
	print("Okay, got it back");
	rename(dest~".temp",dest);
}
