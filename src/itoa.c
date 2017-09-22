#include "itoa.h"

/* itoa: convert n to characters in s */
ssize_t itoa(char s[], size_t space, unsigned int n) {
   int i,j;

   i = 0;
   do {  /* generate digits in reverse order */
			if(i == space) break;
      s[i++] = n % 10 + '0';  /* get next digit */
   } while ((n /= 10) > 0);   /* delete it */

	 // reverse
	 for(j=0;j<i/2;++j) {
		 char t = s[j];
		 s[j] = s[i-j];
		 s[i-j] = t;
	 }
	 return i;
}
