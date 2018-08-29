#include "ensure.h"
#include "storydb.h"

#include <sqlite3.h>
#include "db-private.h"

#include <stdio.h>
#include <stdlib.h> // exit
#include <unistd.h> // chdir
#include <sys/stat.h> // stat


int main(int argc, char *argv[])
{
	bool get_current = argc == 2;
	if(!(get_current || argc == 3)) {
		puts("ready <location> <chapter>");
		exit(1);
	}
	
	const char* location = argv[1];
	long int chapter;
	if(!get_current) {
		chapter = strtol(argv[2], NULL, 0);
		if(chapter == 0) exit(2);
	}
  // make sure we're outside the code directory
	struct stat info;
  while(0 != stat("code",&info)) ensure0(chdir(".."));
	storydb_open();

	size_t loclen = strlen(location);
	if(get_current) {
		DECLARE_STMT(get_ready,
								 "SELECT ready,chapters FROM stories WHERE location = ?1");
		sqlite3_bind_text(get_ready, 1, location, loclen, NULL);
		int res = db_check(sqlite3_step(get_ready));
		if(res == SQLITE_ROW) {
			printf("Story %.*s ready on chapter %d/%d\n",
						 (int)loclen, location,
						 sqlite3_column_int(get_ready, 0),
						 sqlite3_column_int(get_ready, 1));
			exit(0);
		}
		exit(1);
	}
	
	DECLARE_STMT(set_ready,
							 "UPDATE stories SET ready = ?1 WHERE location = ?2 AND ready != ?1");
	sqlite3_bind_int(set_ready, 1, chapter);
	sqlite3_bind_text(set_ready, 2, location, strlen(location), NULL);
	int res = db_check(sqlite3_step(set_ready));
	if(sqlite3_changes(db) == 0) {
		puts("Story already set to that chapter.");
	} else {
		printf("Story %s ready on chapter %ld\n",location, chapter);
	}
	sqlite3_reset(set_ready);
	db_close_and_exit();
	return 0;
}
