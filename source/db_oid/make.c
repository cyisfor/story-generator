#include "base.h"

#include <unistd.h> // chdir, symlink
#include <stdbool.h>
#include <string.h>

#define LITLEN(a) a,sizeof(a)-1

int main(int argc, char *argv[])
{
	chdir("source/db_oid");
	int src;
	bool same = sizeof(db_oid) == sizeof(git_oid);
	if(same) {
		git_oid test = {
			.id = "deadbeefdeadbeef"
		};
		void doit(db_oid res) {
			printf("um %p %p\n",&test.id,res);
			if(&test != res) same = false;
			else if(&test != ((git_oid*)res)) same = false;
			else if(0!=memcmp(test.id,res,sizeof(db_oid))) same = false;
		}
		
		doit(DB_OID(test));
	}
	if(same) {
		unlink("gen.h");
		symlink("same.h","gen.h");
	} else {
		unlink("gen.h");
		symlink("custom.h","gen.h");
	}

	return 0;
}
