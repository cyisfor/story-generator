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
		switch(buf[i]) {
		case '\n':
			PUTLIT("\\n");
		case '\r':
			PUTLIT("\\r");
		case '"':
			PUTLIT("\\\"");
		default:
			write(1,buf+i,1);
		};
	}
	PUTLIT("\";\n");
	
	return 0;
}
