#ifndef _STRING_H_
#define _STRING_H_

#include <stdlib.h> // size_t

typedef struct string {
	char* s;
	size_t l;
} string;

#define STRPRINT(str) fwrite((str).s,(str).l,1,stdout);

#define LITSIZ(a) (sizeof(a)-1)
#define LITLEN(a) a,LITSIZ(a)


#endif /* _STRING_H_ */
