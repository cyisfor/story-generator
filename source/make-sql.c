#include <sys/mman.h>
#include <unistd.h> // write
#include <sys/stat.h>

int main(int argc, char *argv[])
{
	struct stat st;
	if(0!=fstat(0,&st)) return 1;
	char* sql = mmap(NULL,st.st_size,PROT_READ,MAP_PRIVATE,0,0);
	if(sql == MAP_FAILED) return 2;
	close(0);

	#define PUTLIT(a) write(1,a,sizeof(a)-1)

	PUTLIT("const char sql[] = \"");

	size_t i;
	for(i=0;i<st.st_size;++i) {
		switch(sql[i]) {
		case '\n':
			PUTLIT("\\n\"\n\t\"");
		case '\r':
			PUTLIT("\\r");
		case '"':
			PUTLIT("\\\"");
		default:
			write(1,sql+i,1);
		};
	}
	PUTLIT("\";\n");
	
	return 0;
}
