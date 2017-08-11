void infof(const char* file, int flen, int line, const char* fmt, ...);
void warnf(const char* file, int flen, int line, const char* fmt, ...);
void errorf(const char* file, int flen, int line, const char* fmt, ...);
void spamf(const char* file, int flen, int line, const char* fmt, ...);

#define INFO(args...) infof(__FILE__,sizeof(__FILE__)-1,__LINE__, args)
#define WARN(args...) warnf(__FILE__,sizeof(__FILE__)-1,__LINE__, args)
#define ERROR(args...) errorf(__FILE__,sizeof(__FILE__)-1,__LINE__, args)

#ifdef DEBUG
#define SPAM(args...) spamf(__FILE__,__LINE__, args)
#else
#define SPAM(...)
#endif
