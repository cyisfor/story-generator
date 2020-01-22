#define _GNU_SOURCE // memmem
#include "repo.h"
#include "mystring.h"
#include "htmlish.h"
#include "become.h"

#include <libxmlfixes.h> // html_dump_to_fd

#include <libxml/HTMLparser.h> // input
#include <libxml/HTMLtree.h> // output

#include <git2/revwalk.h>
#include <git2/commit.h>
#include <git2/refs.h>

#include <string.h>

#include <stdio.h>
#include <assert.h>


int main(int argc, char *argv[])
{
	if(argc != 2) {
		printf("arg no %d\n",argc);
		exit(1);
	}
	repo_check(repo_discover_init(LITLEN(".git")));

	//libxmlfixes_setup();
	xmlDoc* out = htmlParseFile("template/news-log.html","UTF-8");
	if(out == NULL) {
		printf("Couldn't parse html?");
		exit(2);
	}

	xmlNode* entry_template = fuckXPath((xmlNode*)out, "body");
	if(entry_template == NULL) abort();
	xmlNode* body = entry_template;
	entry_template = xmlCopyNode(entry_template, 1);
	while(body->children) {
		xmlUnlinkNode(body->children);
	}
	xmlDOMWrapCtxtPtr	wrap = xmlDOMWrapNewCtxt();
	
	void entry(time_t time, const char* subject, const char* message) {

		// can't just xmlNodeAddContent because < => &lt;
		
		xmlParserCtxtPtr parser = xmlCreatePushParserCtxt(
			NULL, NULL,
			LITLEN(
				"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
				"<s>"),
			"input.xml");

		int res = xmlParseChunk(parser,
									subject,
									strlen(subject),
									0);
		assert(res == 0);
		res = xmlParseChunk(parser,
									LITLEN("</s>"),
									1);
		assert(res == 0);

		struct {
			xmlNode* subject;
		} inp = {
			parser->myDoc->children
		};
		
		xmlNode* all = xmlCopyNode(entry_template, 1);
		// code
		xmlNode* p1 = nextE(all->children);
		assert(p1);
		xmlNode* p2 = nextE(p1->next);
		assert(p2);
		xmlNode* code = nextE(p1->children);
		assert(code);
		xmlNode* bold = nextE(code->next);
		assert(bold);
		xmlNodeAddContent(code, ctime(&time));

		while(inp.subject->children) {
			xmlNode* c = inp.subject->children;
			xmlUnlinkNode(c);
			c->doc = bold->doc;
			xmlAddChild(bold, xmlCopyNode(c, 1));
		}
		p2->doc = out;
		htmlish_str(p2, message, strlen(message), true);
		p2->doc = NULL;
		xmlAddChild(body, all);

		xmlFreeDoc(parser->myDoc);
    xmlFreeParserCtxt(parser);
	}
	
	int count = 0;

	git_revwalk* walk;
	repo_check(git_revwalk_new(&walk, repo));
	git_revwalk_sorting(walk, GIT_SORT_TIME);

	// since HEAD

	git_revwalk_push_head(walk);

	git_oid oid;

	while(!git_revwalk_next(&oid, walk)) {
		git_commit* commit = NULL;
		repo_check(git_commit_lookup(&commit, repo, &oid));
		const git_signature* sig = git_commit_author(commit);
		int nlen = strlen(sig->name);
		if(nlen == LITSIZ("Skybrook")) {
			if(0==memcmp(sig->name, LITLEN("Skybrook"))) {
				const char* message = git_commit_body(commit);
				if(message != NULL) {
					entry(git_commit_time(commit),
								git_commit_summary(commit),
								message);
					if(++count > 256) break;
				}
			}
		}
	}

	struct becomer* dest = become_start(argv[1]);
	html_dump_to_fd(fileno(dest->out), out);
	become_commit(&dest);

	return 0;
}
