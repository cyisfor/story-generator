#include "db_oid/base.h"
#include "db_oid/gen.h"
#include "ensure.h"
#include "repo.h"
#include "storygit.h"
#include "itoa.h"

#include <unistd.h> // chdir

#include <stdio.h>
#include <sys/stat.h>


int main(int argc, char *argv[])
{
	struct stat info;
	while(0 != stat("code",&info)) ensure0(chdir(".."));
	repo_check(repo_discover_init(LITLEN(".git")));
	int counter = 0;
	enum gfc_action on_commit(
		const db_oid commit,
		const db_oid parent,
		git_time_t timestamp,
		git_tree* last,
		git_tree* cur) {

		fputs(db_oid_str(parent),stderr);
		fputs(" -> ",stderr);
		fputs(db_oid_str(commit),stderr);
		fputc(' ',stderr);
		char buf[0x10];
		fwrite(buf,itoa(buf,0x10,++counter),1,stderr);
		fputc('\n',stderr);

		fputs(ctime(&timestamp),stderr);
		fputc('\n',stderr);
		
		enum gfc_action on_chapter(
			long int chapnum,
			bool deleted,
			const string loc,
			const string src)
		{
			fputc(' ',stderr);
			fwrite(src.base,src.len,1,stderr);
			fputc('\n',stderr);
			return GFC_CONTINUE;
		}
		
		git_for_chapters_changed(last,cur,on_chapter);
		fputc('\n',stderr);
		return GFC_CONTINUE;
	}
	
	git_for_commits(false,0,false,NULL,on_commit);
	return 0;
}
