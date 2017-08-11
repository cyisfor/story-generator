#include "base.h"

#include <sys/sendfile.h>

#include <unistd.h> // chdir
#include <fcntl.h> // open, O_*

int main(int argc, char *argv[])
{
	chdir("source/db_oid");
	int src;
	if(sizeof(db_oid) == sizeof(git_oid)) {
		src = open("same.h",O_RDONLY);
	} else {
		src = open("custom.h",O_RDONLY);
	}
	while(sendfile(src,1) > 0);

	return 0;
}
