#include "db.h"
#include <stdlib.h> // getenv

int main(int argc, char *argv[])
{
	if(argc != 2) return 3;
	db_set_censored(db_find_story(argv[1]),NULL==getenv("unset"));
	return 0;
}
