#include "db_oid/base.h"

#include <unistd.h> // write

int main(int argc, char *argv[])
{
#define PUT(a) write(1,(a),sizeof(a)-1)
#define CHOOSE(a) PUT("#include \"db_oid/" #a ".h\"\n")

	PUT("#include \"db_oid/base.h\"\n");
	if(sizeof(db_oid) == sizeof(git_oid)) {
		CHOOSE(same);
	} else {
		CHOOSE(custom);
	}

	return 0;
}
