#include <stdio.h>
#include <stdlib.h> // abort, getenv
#include <stdarg.h> // va_*


static void note(const char* how, int hlen,
								 const char* file, int flen,
								 int line, const char* fmt, ...) {
	fwrite(how, hlen, 1, stderr);
	fputc(' ',stderr);
	fwrite(file, flen, 1, stderr);
	fprintf(stderr,":%d ",line);
	va_list arg;
	va_start(arg, fmt);
	vfprintf(stderr,fmt,arg);
	va_end(arg);
	fputc('\n',stderr);
}

#define BODY(name) \
	va_list arg;																													\
	va_start(arg, fmt);																										\
	note(name,sizeof(name)-1,file,flen,line,fmt,arg);																					\

#define HEAD(ident) void ident ## f(const char* file, int flen, int line, const char* fmt, ...)

#define DEFINE(ident,name) HEAD(ident) { \
		BODY(name)																																\
}

DEFINE(warn,"DEBUG");
DEFINE(warn,"INFO");
DEFINE(warn,"WARN");

HEAD(error) {
	BODY("ERROR");
	if(getenv("error_nonfatal")) return;
	abort();
}
