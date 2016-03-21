module makers;

import html: Document;

class Maker {
  void make(string src, string dest) const {}
  void chapter(Document doc) const {}
  void contents(Document doc) const {}
}

Maker[string] makers;
