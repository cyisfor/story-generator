#include "base.h"
#include <sys/sendfile.h>

#include <unistd.h> // chdir
#include <fcntl.h> // open, O_*

#define LITLEN(a) a,sizeof(a)-1

int main(int argc, char *argv[])
{
	chdir("source/db_oid");
	int src;
	if(sizeof(db_oid) == sizeof(git_oid)) {
		src = open("same.h",O_RDONLY);
	} else {
		src = open("custom.h",O_RDONLY);
	}
	write(1,LITLEN("#include \"base.h\""));
	while(sendfile(1,src,NULL,0x1000) > 0);

	return 0;
}
