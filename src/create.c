#include "ensure.h"
#include "storydb.h"

#include "ddate.h"
#include "create.h"
#include "htmlish.h"
#include "note.h"

#include "libxmlfixes.h"

#include <libxml/HTMLparser.h> // input
#include <libxml/HTMLtree.h> // output

#include <sys/mman.h> // mmap


#include <fcntl.h> // open
#include <unistd.h> // close
#include <assert.h>
#include <sys/stat.h>
#include <dirent.h> // opendir, DIR, readdir, dirent
#include <string.h> // 

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
	// encoding should be NULL (that indicates UTF-8 in libxml2)
	
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

struct csucks {
	xmlNode* a;
	int chapter;
};

static
void got_title(void* udata, const string title) {
	struct csucks* g = (struct csucks*)udata;
	if(title.base == NULL) {
		char buf[0x100];
		string fallback = {
			.base = buf,
			.len = snprintf(buf,0x100,"Chapter %u",g->chapter)
		};
		xmlNodeAddContentLen(g->a,fallback.base,fallback.len);
	} else
		xmlNodeAddContentLen(g->a,title.base,title.len);
}

struct csucksballs {
	xmlNode* head;
	xmlNode* body;
	string location;
	identifier story;
};

#define IS(what,name) (what && strlen(what)==LITSIZ(name) && 0 == memcmp(what,LITLEN(name)))

static
void do_under_construction(xmlNode* cur) {
	if(!IS(cur->name,"construction")) {
		xmlNode* kid = cur->children;
		while(kid) {
			xmlNode* next = kid->next;
			do_under_construction(kid);
			kid = next;
		}
		return;
	}
	xmlNode* box = xmlNewNode(cur->ns,"div");
	xmlSetProp(box, "class", "construction");
	xmlNode* top = xmlNewNode(cur->ns,"div");
	xmlSetProp(top, "class", "top");
	xmlNode* message = xmlNewNode(cur->ns,"div");
	xmlSetProp(message, "class", "m");
	xmlAddChild(box, top);
	xmlNodeAddContentLen(top, LITLEN("(UNDER CONSTRUCTION)"));
	xmlAddChild(box, message);
	xmlAddNextSibling(cur,box);
	xmlNode* inner = cur->children;
	while(inner) {
		xmlNode* next = inner->next;
		xmlUnlinkNode(inner);
		xmlAddChild(message, inner);
		inner = next;
	}
	xmlUnlinkNode(cur);
}

static
void got_info(
	void* udata,
	string title, string description, string source) {
	struct csucksballs* g = (struct csucksballs*)udata;
	// check for changes first

	bool newdesc = false;
	bool newsource = false;
	bool newtitle = false;

	// check for description file
	char path[0x200];
	memcpy(path,g->location.base,g->location.len);
	memcpy(path+g->location.len,LITLEN("/description\0"));
	int dfd = open(path,O_RDONLY);
	if(dfd >= 0) {
		struct stat st;
		if(0==fstat(dfd,&st)) {
			char* desc = mmap(NULL,st.st_size,PROT_READ,MAP_PRIVATE,dfd,0);
			close(dfd);
			if(description.base &&
			   st.st_size == description.len &&
			   0 == memcmp(desc,description.base,st.st_size)) {
				munmap(desc,st.st_size);
				// description unmodified
			} else {
				newdesc = true;
				// don't forget to munmap!
				description.base = desc;
				description.len = st.st_size;
			}
		}
	}
			
	const char* senv = getenv("source");
	if(senv) {
		size_t len = strlen(senv);
		if(source.base &&
		   len == source.len &&
		   0 == memcmp(source.base,senv,len)) {
			// source unmodified
		} else {
			newsource = true;
			source.base = senv;
			source.len = len;
		}
	}
		
	// title is sometimes a file
	// .../description => .../title
	memcpy(path+g->location.len+1,LITLEN("title\0"));
	int tf = open(path,O_RDONLY);
	if(tf > 0) {
		// eh, should be sorta limited, also saves a stat
		newtitle = true;
		char buf[0x100];
		ssize_t amt = read(tf,buf,0x100);
		if(amt >= 0) {
			buf[amt] = '\0';
			title.base = buf; // goes out of scope when FUNCTION exits
			title.len = amt;
			newtitle = true;
		}
		close(tf);
	}
	
	if(newtitle || newsource || newdesc) {
		storydb_set_info(g->story,title,description,source);
	}
	// if STILL no title, just use location on a temporary unstored basis
	if(!title.base) {
		title = g->location;
	}

	// setup titlehead for htmlish
	char buf[0x100];
	memcpy(buf,title.base,title.len);
	memcpy(buf+title.len,LITLEN(" - \0"));
	setenv("titlehead",buf,1);

	void setup_head(xmlNode* cur) {
		// set <title> and <meta name="description">
		if(!cur) return;
		if(cur->type != XML_ELEMENT_NODE) return setup_head(cur->next);
			
		if(IS(cur->name,"title")) {
			if(title.base)
				xmlNodeAddContentLen(cur,title.base,title.len);
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
									xmlNodeAddContentLen((xmlNode*)attr,description.base,description.len);
									return;
								}
							}
							// no content attribute found... create one
							attr = xmlSetProp(cur, "content","");
							// then add to it
							xmlNodeAddContentLen((xmlNode*)attr,description.base,description.len);
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
			if(title.base) {
				xmlNode* new = xmlNewTextLen(title.base,title.len);
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
		} else if(description.base && IS(cur->name,"div")) {
			xmlAttr* attr;
			for(attr=cur->properties;attr;attr=attr->next) {				
				if(IS(attr->name,"id")) {
					if(IS(attr->children->content,"description")) {
						// in the <body> description will be in htmlish...
						htmlish_str(cur,description.base,description.len,true);
						// don't descend into it looking for intitle and stuff?
						// what about a description that contains a description div???
						break;
					}
				}
			}
		} else if(source.base && IS(cur->name,"source")) {
			// move contents into an anchor node
			xmlNode* a = xmlNewNode(cur->ns,"a");
			// since libxml sucks, source.base must be null terminated
			xmlSetProp(a,"href",source.base);
			xmlAddChild(a,cur->children);
			xmlReplaceNode(cur,a);
			xmlFreeNode(cur);
			return setup_body(a->next);
		}
		setup_body(cur->children);
		return setup_body(cur->next);
	}
	if(title.base || description.base) {
		setup_head(g->head);
		setup_body(g->body);		
	} else if(source.base) {
		setup_body(g->body);
	}
	if(newdesc) {
		munmap((char*)description.base,description.len);
	}
}

bool mycheck(xmlNode* cur, const string element, const string id) {
	if(strlen(cur->name) != element.len) return false;
	if(0!=memcmp(cur->name, element.base, element.len)) return false;
	if(!xmlHasProp(cur,"id")) return false;
	xmlChar* val = xmlGetProp(cur,"id");
	if(strlen(val) != id.len) return false;
	bool yes = 0 == memcmp(val,id.base,id.len);
	xmlFree(val);
	return yes;
}

int create_contents(identifier story,
					const string title_file,
					const string location,
					int dest,
					size_t chapters) {
	xmlDoc* doc = xmlCopyDoc(contents_template,1);
	
	// root, doctype, html, text prefix, head
	xmlNode* head = doc->children->next->children->next;
	xmlNode* body = head->next->next;
	xmlNode* image = NULL;
	xmlNode* toc = NULL;
	void find_stuff(xmlNode* cur) {
		if(!cur) return;
		if(cur->type == XML_ELEMENT_NODE) {
			if(mycheck(cur, LITSTR("ol"), LITSTR("toc"))) {
				toc = cur;
				if(image) return;
			} else if(mycheck(cur, LITSTR("img"), LITSTR("title"))) {
				image = cur;
				if(toc) return;
			}
		}

		find_stuff(cur->children);
		if(image && toc) return;
		return find_stuff(cur->next);
	}

	find_stuff(body);
	if(!(image && toc)) {
		htmlDocDump(stdout,doc);
		abort();
	}

	xmlSetProp(image, "src", title_file.base);
	
	size_t i;
	for(i=1;i<=chapters;++i) {
		xmlNode* li = xmlNewNode(toc->ns,"li");
		struct csucks g = {
			.a = xmlNewNode(li->ns,"a"),
			.chapter = i
		};
		xmlAddChild(li,g.a);
		xmlAddChild(toc,li);
		CHAPTER_NAME(i,buf);
		xmlSetProp(g.a,"href",buf);

		storydb_with_chapter_title(
			&g,
			got_title,
			story, i);
	}

	{
		struct csucksballs g = {
			.head = head,
			.body = body,
			.location = location,
			.story = story
		};
		storydb_with_info(&g, got_info, story);
	}

	unsetenv("titlehead");

	html_dump_to_fd(dest,doc);
	
	return chapters;
}

void create_chapter(int src, int dest,
					int chapter, int ready,
					identifier story, bool* title_changed) {

	xmlDoc* doc = xmlCopyDoc(chapter_template,1);
	bool as_child = false;
	xmlNode* content = getContent(xmlDocGetRootElement(doc),false,&as_child);

	db_working_on(story,chapter);
	htmlish(content,src,as_child);
	
	if(!as_child) {
		// throw away placeholder node
		xmlUnlinkNode(content);
		xmlFreeNode(content);
	}

	// root, doctype, html, text prefix, head
	xmlNode* head = nextE(doc->children->next->children);
	// head, blank, body
	xmlNode* body = head->next->next;
	do_under_construction(body);

	// text suffix, body, last e in body
	xmlNode* links = head->next->next->last;
	while(links->type != XML_ELEMENT_NODE) {
		links = links->prev;
		assert(links);
	}
	xmlNode* title = get_title(head);
	if(title && title->children) {
		string t = {
			.base = title->children->content
		};
		t.len = strlen(t.base);
		storydb_set_chapter_title(t, story, chapter, title_changed);
	} /*else {
		WARN("no chapter title found for %d %d",story,chapter);
		}*/

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
		if(chapter != 2) {
			snprintf(buf,0x100,"chapter%d.html",chapter-1);
			//otherwise just use index.html
		}

		linkthing(buf,"prev",LITLEN("Prev"));
	}

	xmlNodeAddContentLen(links,LITLEN(" "));
	linkthing("contents.html","first",LITLEN("Contents"));

	// XXX: chapters - finished ? 0 : 1
	if(chapter < ready) {
		xmlNodeAddContentLen(links,LITLEN(" "));
		snprintf(buf,0x100,"chapter%d.html",chapter+1);
		linkthing(buf,"next",LITLEN("Next"));
	}
	set_created(head->next->next);

	html_dump_to_fd(dest,doc);
}
