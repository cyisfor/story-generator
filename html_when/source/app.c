#include "ensure.h"
#include "html_when.h"
#include "output.h"
#include "gumbo.h"
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(int argc, char**argv) {

	struct stat info;
	ensure0(fstat(0,&info));
	char* buf = mmap(NULL,info.st_size,PROT_READ,MAP_PRIVATE,0,0);
	ensure_ne(MAP_FAILED,buf);
	GumboOutput* out = gumbo_parse_with_options(&kGumboDefaultOptions, buf, info.st_size);
	html_when(out->document);
	int dest = open("/tmp/output.deleteme",O_WRONLY|O_CREAT|O_TRUNC,0644);
	output(dest,out->document);
	munmap(buf,info.st_size);
}
