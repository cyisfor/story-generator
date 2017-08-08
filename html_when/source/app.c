#include "ensure.h"
#include "html_when.h"
#include "output.h"
#include "gumbo.h"
#include <sys/mman.h>
#include <sys/stat.h>

int main(int argc, char**argv) {

	struct stat info;
	ensure0(fstat(0,&info));
	char* buf = mmap(NULL,info.st_size,PROT_READ,MAP_PRIVATE,0,0);
	ensure_ne(MAP_FAILED,buf);
	GumboOutput* out = gumbo_parse_with_options(&kGumboDefaultOptions, buf, info.st_size);
	munmap(buf,info.st_size);
	html_when(out);
	output(1,out);
}
