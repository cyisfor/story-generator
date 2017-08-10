typedef struct string {
	char* s;
	unsigned short l;
} string;

#define STRPRINT(str) fwrite((str).s,(str).l,1,stdout);
