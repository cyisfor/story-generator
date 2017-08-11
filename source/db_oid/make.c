#include "base.h"

#include <unistd.h> // chdir, symlink
#include <stdbool.h>

#define LITLEN(a) a,sizeof(a)-1

int main(int argc, char *argv[])
{
	chdir("source/db_oid");
	int src;
	bool same = sizeof(db_oid) == sizeof(git_oid);
	if(same) {
		git_oid testt = {
			.id = "deadbeefdeadbeefdeadbeef"
		};
		db_oid res = DB_OID(testt);
		if(&testt != &res) same = false;
		else if(&testt != ((git_oid*)&res)) same = false;
		else if(0!=memcmp(testt.id,res,sizeof(db_oid))) same = false;
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
