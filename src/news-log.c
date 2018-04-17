#define _GNU_SOURCE // memmem
#include "repo.h"
#include "mystring.h"

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

	libxmlfixes_setup();
	xmlDoc* out = htmlParseFile("template/news-log.html","UTF-8");
	if(out == NULL) {
		printf("Couldn't parse html?");
		exit(2);
	}

	xmlNode* entry_template = fuckXPath(out, "body");
	if(entry_template == NULL) abort();
	xmlNode* body = entry_template;
	entry_template = xmlCopyNode(entry_template, 1);
	while(body->children) {
		xmlUnlinkNode(body->children);
	}
	void entry(time_t time, const char* subject, const char* message) {
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
		xmlNodeAddContent(bold, subject);
		p2->doc = out;
		htmlish_str(p2, message, strlen(message), true);
		p2->doc = NULL;
		xmlAddChild(body, all);
	};
	
	int count = 0;

	git_revwalk* walk;
	repo_check(git_revwalk_new(&walk, repo));
	git_revwalk_sorting(walk, GIT_SORT_TIME);

	// since HEAD
	git_reference* ref;
	repo_check(git_repository_head(&ref, repo)); // this resolves the reference
	const git_oid * oiderp = git_reference_target(ref);
	git_reference_free(ref);

	git_revwalk_push(walk, oiderp);

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
