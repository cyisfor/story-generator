#include <stdio.h> // perror

#define ensure0(test) if((test) != 0) { perror(#test); abort(); }
