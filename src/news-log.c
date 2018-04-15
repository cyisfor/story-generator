#include "repo.h"
#include <stdio.h>


int main(int argc, char *argv[])
{
	repo_check(repo_discover_init(LITLEN(".git")));

	git_revwalk* walk;
	repo_check(git_revwalk_new(&walk, repo));
	git_revwalk_sorting(GIT_SORT_TIME);

	puts("<!DOCTYPE html>\n"
			 "<html><head><meta charset=\"utf-8\"/>\n"
			 "<title>News log</title>\n"
			 "</head><body>\n");
	
	// since HEAD
	git_reference* ref;
	repo_check(git_repository_head(&ref, repo)); // this resolves the reference
	const git_oid * oid = git_reference_target(ref);
	git_reference_free(ref);

	git_revwalk_push(walk, oid);

	while(!git_revwalk_next(&oid, walk)) {
		git_commit* commit = NULL;
		repo_check(git_commit_lookup(&commit, repo, oid));
		const git_signature* sig = git_commit_author(commit);
		int nlen = strlen(sig->name);
		if(nlen == sizeof("Skybrook")-1) {
			if(0==memcmp(sig->name, "Skybrook", nlen)) {
				const char* message = git_commit_body(commit);
				if(message != NULL) {
					puts("<p>");
					time_t time = git_commit_time(commit);
					puts(ctime(&time));
					puts("<b>");
					puts(git_commit_summary(commit));
					puts("</b></p><p>");
					puts(message);
					puts("</p>");
					if(++count > 256) break;
				}
			}
		}
	}
	puts("</body></html>");
	return 0;
}
