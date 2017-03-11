module makers;

import html: Document;

alias Make = void delegate(string src, string dest);
Make[string] make;
alias Fiddler = void delegate(Document* doc);
Fiddler[string] contents;
Fiddler[string] chapter;

