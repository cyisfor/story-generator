#include <sqlite3.h>

#include "db-private.h"
#include "storydb.h"

#include <stdio.h>
#include <stdlib.h> // exit
#include <unistd.h> // chdir
#include <sys/stat.h> // stat


int main(int argc, char *argv[])
{
	if(argc != 3) exit(1);
	const char* location = argv[1];
	long int chapter = strtol(argv[2], NULL, 0);
	if(chapter == 0) exit(2);
  // make sure we're outside the code directory
	struct stat info;
  while(0 != stat("code",&info)) chdir("..");
	storydb_open();

	DECLARE_STMT(set_ready,
							 "UPDATE stories SET ready = MAX(ready,?) WHERE location = ?");
	sqlite3_bind_int(set_ready, 1, chapter);
	sqlite3_bind_text(set_ready, 2, location, strlen(location), NULL);
	int res = db_check(sqlite3_step(set_ready));
	if(sqlite3_changes(db) == 0) {
		puts("Story already set to that chapter.");
	} else {
		printf("Story %s ready on chapter %ld\n",location, chapter);
	}
	return 0;
}
