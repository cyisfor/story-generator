#include "ddate.h"
#include "create.h"
#include "htmlish.h"
#include <libxml/HTMLparser.h> // input
#include <libxml/HTMLtree.h> // output
#include <fcntl.h> // open
#include <unistd.h> // close
#include <assert.h>
#include <sys/stat.h>
#include <dirent.h> // opendir, DIR, readdir, dirent
#include <string.h> // 

static bool AISOLDER(struct stat a, struct stat b) {
	if(a.st_mtime < b.st_mtime) return true;
	if(a.st_mtime == b.st_mtime) return false;
	return a.st_mtim.tv_nsec < b.st_mtim.tv_nsec;
}

static void set_created(xmlNode* body) {
	xmlNode* div = xmlNewNode(body->ns, "div");
	xmlSetProp(div,"id","ddate");
	xmlNodeAddContentLen(div,LITLEN("This page was created on "));
	
	char buf[0x400];
	time_t now = time(NULL);
	int amt = disc_format(buf,0x400,NULL,disc_fromtm(gmtime(&now)));
	xmlNodeAddContentLen(div,buf,amt);
	xmlAddChild(body,div);
}

xmlDoc* chapter_template = NULL;
xmlDoc* contents_template = NULL;

const char defaultTemplate[] =
  "<!DOCTYPE html>\n"
  "<html>\n"
  "<head><meta charset=\"utf-8\"/>\n"
  "<title/></head>\n"
  "<body>\n"
  "<content/>\n"
  "</body></html>";

void create_setup(void) {
	chapter_template = htmlParseFile("template/chapter.html","UTF-8");
	if(!chapter_template) {
		chapter_template = htmlReadMemory(LITLEN(defaultTemplate),
																			"","utf-8",
																			HTML_PARSE_RECOVER |
																			HTML_PARSE_NOBLANKS |
																			HTML_PARSE_COMPACT);
	}
	contents_template = htmlParseFile("template/contents.html","UTF-8");
	if(!contents_template) {
		contents_template = chapter_template;
	}
}

int create_contents(const string location,
										const string dest,
										size_t chapters,
										void (*with_title)(identifier chapter,
																			 void(*handle)(const string title))) {	
	xmlDoc* doc = xmlCopyDoc(contents_template,1);
	// root, doctype, html, text prefix, head
	xmlNode* head = doc->children->next->children->next;
	xmlNode* body = head->next->next;
	xmlNode* find_toc(xmlNode* cur) {
		if(!cur) return NULL;
		if(cur->type == XML_ELEMENT_NODE) {
			if(0==strcmp(cur->name,"ol")) {
				if(xmlHasProp(cur,"id")) {
					xmlChar* val = xmlGetProp(cur,"id");
					bool yes = 0 == strcmp(val,"toc");
					xmlFree(val);
					if(yes) {
						return cur;
					}
				}
			}
		}

		xmlNode* test = find_toc(cur->children);
		if(test) return test;
		return find_toc(cur->next);
	}

	xmlNode* toc = find_toc(body);
	if(!toc) {
		htmlDocDump(stdout,doc);
	}
	assert(toc);
	
	size_t i;
	for(i=0;i<chapters;++i) {
		xmlNode* li = xmlNewNode(toc->ns,"li");
		xmlNode* a = xmlNewNode(li->ns,"a");
		xmlAddChild(li,a);
		xmlAddChild(toc,li);
		char buf[0x100] = "index.html";
		if(i > 0) {
			snprintf(buf,0x100,"chapter%d.html",i);
		}
		xmlSetProp(a,"href",buf);
		void got_title(const string title) {
			xmlNodeAddContentLen(a,title.s,title.l);
			xmlNode* get_title(xmlNode* cur) {
				if(!cur) return NULL;
				if(cur->type == XML_ELEMENT_NODE &&
					 strlen(cur->name)==LITSIZ("title") &&
					 0==memcmp(cur->name,LITLEN("title"))) return cur;
				xmlNode* t = get_title(cur->children);
				if(t) return t;
				return get_title(cur->next);
			}
			xmlNode* t = get_title(head);
			if(t) {
				xmlNodeAddContentLen(t,title.s,title.l);
			}
		}
		with_title(i, got_title);
	}

	htmlSaveFileEnc(dest.s,doc,"UTF-8");
	
	return chapters;
}

void create_chapter(string src, string dest, int chapter, int chapters) {
	int srcfd = open(src.s,O_RDONLY);
	if(srcfd < 0) {
		printf("%s moved...\n",src.s);
		return;
	}
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

	xmlDoc* doc = xmlCopyDoc(chapter_template,1);
	bool as_child = false;
	xmlNode* content = getContent(xmlDocGetRootElement(doc),false,&as_child);
	htmlish(content,srcfd,as_child);
	close(srcfd);
	if(!as_child) {
		// throw away placeholder node
		xmlUnlinkNode(content);
		xmlFreeNode(content);
	}

	// root, doctype, html, text prefix, head
	xmlNode* head = doc->children->next->children->next;
	// text suffix, body, last e in body
	xmlNode* links = head->next->next->last;
	while(links->type != XML_ELEMENT_NODE) {
		links = links->prev;
		assert(links);
	}

	char buf[0x100] = "index.html";
	if(chapter > 1) {
		if(chapter != 1) {
			snprintf(buf,0x100,"chapter%d.html",chapter-1);
			//otherwise just use index.html
		}

		xmlNode* a = xmlNewNode(links->ns,"a");
		xmlSetProp(a,"href",buf);
		xmlNodeAddContent(a,"Prev");
		xmlAddChild(links,a);
		a = xmlNewNode(head->ns,"link");
		xmlSetProp(a,"rel","prev");
		xmlSetProp(a,"href",buf);
		xmlAddChild(head,a);
	}
	if(chapter < chapters) {
		xmlNodeAddContent(links," ");
		snprintf(buf,0x100,"chapter%d.html",chapter+1);
		xmlNode* a = xmlNewNode(links->ns,"a");
		xmlSetProp(a,"href",buf);
		xmlNodeAddContent(a,"Next");
		xmlAddChild(links,a);
		a = xmlNewNode(head->ns,"link");
		xmlSetProp(a,"rel","next");
		xmlSetProp(a,"href",buf);
		xmlAddChild(head,a);
	}
	set_created(head->next->next);

	htmlSaveFileEnc(dest.s,doc,"UTF-8");
}
