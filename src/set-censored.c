#include "db.h"
#include <sys/stat.h>

#include <stdlib.h> // getenv
#include <unistd.h> // chdir

int main(int argc, char *argv[])
{
	if(argc != 2) return 3;
	struct stat info;
	while(0 != stat("code",&info)) chdir("..");
	db_open("generate.sqlite");
	db_set_censored(db_find_story(ztos(argv[1])),
									NULL==getenv("unset"));
	return 0;
}
