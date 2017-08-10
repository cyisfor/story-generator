typedef struct string {
	char* s;
	unsigned short l;
} string;

#define STRPRINT(str) fwrite((str).s,(str).l,1,stdout);

#define LITSIZ(a) (sizeof(a)-1)
#define LITLEN(a) a,LITSIZ(a)
