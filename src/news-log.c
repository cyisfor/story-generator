#include "repo.h"
#include "mystring.h"

#include <git2/revwalk.h>
#include <git2/commit.h>
#include <git2/refs.h>


#include <stdio.h>


int main(int argc, char *argv[])
{
	int count = 0;
	repo_check(repo_discover_init(LITLEN(".git")));

	git_revwalk* walk;
	repo_check(git_revwalk_new(&walk, repo));
	git_revwalk_sorting(walk, GIT_SORT_TIME);

	puts("<!DOCTYPE html>\n"
			 "<html><head><meta charset=\"utf-8\"/>\n"
			 "<title>News log</title>\n"
			 "</head><body>\n");
	
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
					puts("<p>");
					time_t time = git_commit_time(commit);
					puts(ctime(&time));
					puts("<b>");
					puts(git_commit_summary(commit));
					puts("</b></p><p>");
					const char* cur = message;
					size_t mlen = strlen(message);
					for(;;) {
						const char* nl = strstr(cur,"\n\n");
						int done = nl == NULL;
						if(done) {
							nl = message + mlen;
						} 
							
						puts("<p>");
						fwrite(cur, nl-cur, 1, stdout);
						puts("</p>");
						if(done) {
							break;
						}
					}
							
					puts("</p>");
					if(++count > 256) break;
				}
			}
		}
	}
	puts("</body></html>");
	return 0;
}
