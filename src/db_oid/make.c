#include "base.h"

#include <unistd.h> // chdir, symlink
#include <stdbool.h>
#include <string.h>

#define LITLEN(a) a,sizeof(a)-1

int main(int argc, char *argv[])
{
	if(argc != 3) exit(4);

	const char* srcdir = realpath(argv[2],NULL);
	int srcdirlen= strlen(srcdir);
	
	const char* destdir = argv[1];
	if(chdir(destdir)) exit(3);
	
	int src;
	bool same = sizeof(db_oid) == sizeof(git_oid);
	if(same) {
		git_oid test = {
			.id = "deadbeefdeadbeef"
		};
		void doit(db_oid res) {
			if((void*)&test != (void*)res) same = false;
			else if(&test != res) same = false;
			else if(&test != ((git_oid*)res)) same = false;
			else if(0!=memcmp(test.id,res,sizeof(db_oid))) same = false;
		}
		
		doit(DB_OID(test));
	}
	char* buf = malloc(srcdirlen + 10);
	memcpy(buf, srcdir, srcdirlen);
	free(srcdir);
	buf[srcdirlen] = '/';
	if(same) {
		memcpy(buf+srcdirlen+1, "same.h\0", 7);
	} else {
		memcpy(buf+srcdirlen+1, "custom.h\0", 9);
	}
	unlink("gen.h");
	symlink(buf,"gen.h");

	return 0;
}
