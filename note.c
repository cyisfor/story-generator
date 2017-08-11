static void log(const char* file, int line, const char* fmt, ...) {
	fputs(file, stderr);
	printf(":%d ",line);
	va_list arg;
	va_start(fmt,arg);
	vfprintf(stderr,fmt,arg);
	va_end(arg);
}

void infof(const char* file, int line, const char* fmt, ...) {
	fputs("INFO: ",stderr);
	log(file,line,fmt);
}
void warnf(const char* file, int line, const char* fmt, ...) {
	fputs("WARN: ",stderr);
	log(file,line,fmt);
}
void errorf(const char* file, int line, const char* fmt, ...) {
	fputs("ERROR: ",stderr);
	log(file,line,fmt);
	if(getenv("error_nonfatal")) return;
	abort();
}
