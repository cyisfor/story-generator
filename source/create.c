#include "create.h"
#include "htmlish.h"
#include <libxml/HTMLparser.h> // input
#include <libxml/HTMLtree.h> // output
#include <fcntl.h> // open
#include <unistd.h> // close
#include <assert.h>

xmlDoc* chapter_template = NULL;

const char defaultTemplate[] =
  "<!DOCTYPE html>\n"
  "<html>\n"
  "<head><meta charset=\"utf-8\"/>\n"
  "<title/><header/></head>\n"
  "<body><h1><intitle/></h1>\n"
  "<top/><content/><footer/>\n"
  "</body></html>";

void create_setup(void) {
	chapter_template = htmlReadMemory(LITLEN(defaultTemplate),
																		"","utf-8",
																		HTML_PARSE_RECOVER |
																		HTML_PARSE_NOBLANKS |
																		HTML_PARSE_COMPACT);
}

void create_chapter(string src, string dest) {
	int srcfd = open(src.s,O_RDONLY);
	assert(srcfd >= 0);
	struct stat srcinfo;
	assert(0==fstat(srcfd,&srcinfo));
	struct stat destinfo;
	bool dest_exists = 0==stat(dest.s,&destinfo);
	if(dest_exists && !AISOLDER(destinfo,srcinfo)) {
		fputs("skip ",stdout);
		STRPRINT(src);
		fputc('\n',stdout);
		return;
	}
	if(!dest_exists) {
		puts("warning dest no exist!");
	}
	fputs("then create uh ",stdout);
	STRPRINT(src);
	fputs(" -> ",stdout);
	STRPRINT(dest);
	fputc('\n',stdout);

	xmlDoc* doc = xmlCopyDoc(template,1);
	htmlish(doc,srcfd);
	close(srcfd);
	htmlDocSaveFileEnc(dest.s,doc,"UTF-8");
}
