#include <stdio.h>
#include <stdlib.h> // abort, getenv
#include <stdarg.h> // va_*


static void note(const char* file, int line, const char* fmt, ...) {
	fputs(file, stderr);
	fprintf(stderr,":%d ",line);
	va_list arg;
	va_start(arg, fmt);
	vfprintf(stderr,fmt,arg);
	va_end(arg);
	fputc('\n',stderr);
}

void infof(const char* file, int line, const char* fmt, ...) {
	fputs("INFO ",stderr);
	note(file,line,fmt);
}
void warnf(const char* file, int line, const char* fmt, ...) {
	fputs("WARN ",stderr);
	note(file,line,fmt);
}
void spamf(const char* file, int line, const char* fmt, ...) {
	fputs("DEBUG ",stderr);
	note(file,line,fmt);
}
void errorf(const char* file, int line, const char* fmt, ...) {
	fputs("ERROR ",stderr);
	note(file,line,fmt);
	if(getenv("error_nonfatal")) return;
	abort();
}
