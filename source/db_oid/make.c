#include "base.h"

#include <unistd.h> // chdir, symlink

#define LITLEN(a) a,sizeof(a)-1

int main(int argc, char *argv[])
{
	chdir("source/db_oid");
	int src;
	if(sizeof(db_oid) == sizeof(git_oid)) {
		unlink("gen.h");
		symlink("same.h","gen.h");
	} else {
		unlink("gen.h");
		symlink("custom.h","gen.h");
	}

	return 0;
}
