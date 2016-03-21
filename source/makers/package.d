module makers;

import html: Document;

alias Make = void function(string src, string dest);
Make[string] make;
alias Fiddler = void function(ref Document doc);
Fiddler[string] contents;
Fiddler[string] chapter;

