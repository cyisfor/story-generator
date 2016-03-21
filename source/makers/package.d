module makers;

import html: Document;

class Maker {
  void make(string src, string dest) {}
  void chapter(Document doc) {}
  void contents(Document doc) {}
}

Maker[string] makers;
