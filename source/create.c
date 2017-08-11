#include "ddate.h"
#include "create.h"
#include "htmlish.h"
#include <libxml/HTMLparser.h> // input
#include <libxml/HTMLtree.h> // output

#include <sys/mman.h> // mmap


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

static xmlNode* get_title(xmlNode* cur) {
	if(!cur) return NULL;
	if(cur->type == XML_ELEMENT_NODE &&
		 strlen(cur->name)==LITSIZ("title") &&
		 0==memcmp(cur->name,LITLEN("title"))) {
		return cur;
	}
	xmlNode* t = get_title(cur->children);
	if(t) return t;
	return get_title(cur->next);
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

int create_contents(identifier story,
										const string location,
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
		}
		with_title(i, got_title);
	}
	
	void got_info(string title, string description, string source) {

		// check for changes first

		bool newdesc = false;
		bool newsource = false;
		bool newtitle = false;

		// check for description file
		char path[0x200];
		memcpy(path,location.s,location.l);
		memcpy(path+location.l,LITLEN("/description\0"));
		int dfd = open(path,O_RDONLY);
		if(dfd >= 0) {
			struct stat st;
			if(0==fstat(dfd,&st)) {
				char* desc = mmap(NULL,st.st_size,PROT_READ,MAP_PRIVATE,dfd,0);
				close(dfd);
				if(description.s &&
					 st.st_size == description.l &&
					 0 == memcmp(desc,description.s,st.st_size)) {
					munmap(desc,st.st_size);
					// description unmodified
				} else {
					newdesc = true;
					// don't forget to munmap!
					description.s = desc;
					description.l = st.st_size;
				}
			}
		}
			
		const char* senv = getenv("source");
		if(senv) {
			size_t len = strlen(senv);
			if(source.s &&
				 len == source.l &&
				 0 == memcmp(source.s,senv,len)) {
				// source unmodified
			} else {
				newsource = true;
				source.s = senv;
				source.l = len;
			}
		}

		// we process many stories, so using one title for all of them is stupid...
		const char* tenv = getenv("title");
		if(tenv) {
			size_t len = strlen(tenv);
			if(title.s &&
				 len == title.l &&
				 0 == memcmp(title.s,tenv,len)) {
				// title unmodified
			} else {
				newtitle = true;
				title.s = tenv;
				title.l = len;
			}
		} else {
			// title is sometimes a file
			// .../description => .../title
			memcpy(path+location.l+1,LITLEN("title\0"));
			int tf = open(path,O_RDONLY);
			if(tf > 0) {
				// eh, should be sorta limited, also saves a stat
				newtitle = true;
				char buf[0x100];
				ssize_t amt = read(tf,buf,0x100);
				if(amt >= 0) {
					buf[amt] = '\0';
					title.s = buf; // goes out of scope when FUNCTION exits
					title.l = amt;
					newtitle = true;
				}
				close(tf);
			}
		}

		if(newtitle || newsource || newdesc) {
			db_set_story_info(story,title,description,source);
		}

		// if STILL no title, just use location on a temporary unstored basis
		if(!title.s) {
			title = location;
		}

		// setup titlehead for htmlish
		char buf[0x100];
		memcpy(buf,title.s,title.l);
		memcpy(buf+title.l,LITLEN(" - \0"));
		setenv("titlehead",buf,1);

#define IS(what,name) what && strlen(what)==LITSIZ(name) && 0 == memcmp(what,LITLEN(name)) 
		void setup_head(xmlNode* cur) {
			// set <title> and <meta name="description">
			if(!cur) return;
			if(cur->type != XML_ELEMENT_NODE) return setup_head(cur->next);
			
			if(IS(cur->name,"title")) {
				if(title.s)
					xmlNodeAddContentLen(cur,title.s,title.l);
			} else if(IS(cur->name,"meta")) {
				xmlAttr* attr;
				for(attr=cur->properties;attr;attr=attr->next) {
					if(IS(attr->name,"name")) {
						if(IS(attr->children->content,"description")) {
							void check(void) {
								xmlAttr* attr;
								// start over (we don't care about remaining attrs now
								for(attr=cur->properties;attr;attr=attr->next) {
									if(IS(attr->name,"content")) {
										// can't use xmlSetProp because mmap has no null terminator
										xmlNodeAddContentLen((xmlNode*)attr,description.s,description.l);
										return;
									}
								}
								// no content attribute found... create one
								attr = xmlSetProp(cur, "content","");
								// then add to it
								xmlNodeAddContentLen((xmlNode*)attr,description.s,description.l);
								return;
							}
							check();
							break;
						}
					}
				}
			}
			setup_head(cur->children);
			return setup_head(cur->next);
		}

		void setup_body(xmlNode* cur) {
			if(!cur) return;
			if(cur->type != XML_ELEMENT_NODE) return setup_body(cur->next);
			
			if(IS(cur->name,"intitle")) {
				if(title.s) {
					xmlNode* new = xmlNewTextLen(title.s,title.l);
					xmlReplaceNode(cur,new);
					// no need to check a text node's children
					return setup_body(new->next);
				} else {
					// remove it anyway
					xmlNode* next = cur->next;
					xmlUnlinkNode(cur);
					xmlFreeNode(cur);
					return setup_body(next);
				}
			} else if(description.s && IS(cur->name,"div")) {
				xmlAttr* attr;
				for(attr=cur->properties;attr;attr=attr->next) {				
					if(IS(attr->name,"id")) {
						if(IS(attr->children->content,"description")) {
							// in the <body> description will be in htmlish...
							htmlish_str(cur,description.s,description.l,true);
							// don't descend into it looking for intitle and stuff?
							// what about a description that contains a description div???
							break;
						}
					}
				}
			} else if(source.s && IS(cur->name,"source")) {
				// move contents into an anchor node
				xmlNode* a = xmlNewNode(cur->ns,"a");
				// since libxml sucks, source.s must be null terminated
				xmlSetProp(a,"href",source.s);
				xmlAddChild(a,cur->children);
				xmlReplaceNode(cur,a);
				xmlFreeNode(cur);
				return setup_body(a->next);
			}
			setup_body(cur->children);
			return setup_body(cur->next);
		}
		if(title.s || description.s) {
			setup_head(head);
			setup_body(body);		
		} else if(source.s) {
			setup_body(body);
		}
		if(newdesc) {
			munmap((char*)description.s,description.l);
		}
	}
	db_with_story_info(story, got_info);

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
	// head, blank, body
	xmlNode* body = head->next->next;
	// text suffix, body, last e in body
	xmlNode* links = head->next->next->last;
	while(links->type != XML_ELEMENT_NODE) {
		links = links->prev;
		assert(links);
	}

	void linkthing(const char* href, const char* rel, const char* title, size_t tlen) {
		xmlNode* a = xmlNewNode(links->ns,"a");
		xmlSetProp(a,"href",href);
		xmlNodeAddContentLen(a,title,tlen);
		xmlAddChild(links,a);
		a = xmlNewNode(head->ns,"link");
		xmlSetProp(a,"rel",rel);
		xmlSetProp(a,"href",href);
		xmlAddChild(head,a);
	}
	
	char buf[0x100] = "index.html";
	if(chapter > 1) {
		if(chapter != 1) {
			snprintf(buf,0x100,"chapter%d.html",chapter-1);
			//otherwise just use index.html
		}

		linkthing(buf,"prev",LITLEN("Prev"));
	}
	

	xmlNodeAddContentLen(links,LITLEN(" "));
	linkthing("contents.html","first",LITLEN("Contents"));
	
	if(chapter < chapters) {
		xmlNodeAddContentLen(links,LITLEN(" "));
		snprintf(buf,0x100,"chapter%d.html",chapter+1);
		linkthing(buf,"next",LITLEN("Next"));
	}
	set_created(head->next->next);

	htmlSaveFileEnc(dest.s,doc,"UTF-8");
}
