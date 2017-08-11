void infof(const char* file, int line, const char* fmt, ...);
void warnf(const char* file, int line, const char* fmt, ...);
void errorf(const char* file, int line, const char* fmt, ...);

#define info(args...) infof(__FILE__,__LINE__, args)
#define warn(args...) warnf(__FILE__,__LINE__, args)
#define error(args...) errorf(__FILE__,__LINE__, args)
