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


int main(int argc, char *argv[])
{
	if(argc != 2) abort();

	xmlDoc* out = htmlParseFile("template/news-log.html","UTF-8");
	if(out == NULL) abort();

	xmlNode* entry_template = fuckXPathDivId(out->children, "entry");
	if(entry_template == NULL) abort();
	xmlNode* body = entry_template->parent;
	xmlUnlinkNode(entry_template);
	void entry(time_t time, const char* subject, const char* message) {
		xmlNode* all = xmlCopyNode(entry_template, 1);
		// code
		xmlNodeAddContent(all->children, ctime(&time));
		xmlNodeAddContent(all->children->next, subject);
		htmlish_str(all->children->last, message, strlen(message), true);
		xmlNodeAddChild(body, all);
	};
	
	int count = 0;
	repo_check(repo_discover_init(LITLEN(".git")));

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
