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

import fuck_selectors: for_all, is_element, by_name, by_class, by_id;

auto process_root(Document dest) {
  return (auto node) {
	
  
auto parse(HTMLstring source, ref Document templit = null) {
  import std.algorithm.searching: until;
  import std.range: chain;
  import htmlderp: createDocument;
  
  if(templit == null) {
	templit = default_template;
  }

  auto dest = templit.clone();
  
  // find where we're going to dump this htmlish
  auto content = until!"a !is null"
	(chain(dest.root.by_name!"content",
		   dest.root.by_name!"div".by_id!"content",
		   dest.root.by_name!"body")).front;
  if(content is null) {
	content = dest.createElement("body");
	dest.root.appendChild(content);
  }
  
  auto src = createDocument(source);
  assert(src.root);

  content.has_attr!"hish".each!process_root(dest);
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
