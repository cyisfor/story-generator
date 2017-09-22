/* itoa: convert n to characters in s */
bool itoa(char s[], size_t space, unsigned int n) {
   int i,j;

   i = 0;
   do {  /* generate digits in reverse order */
			if(i == space) return false;
      s[i++] = n % 10 + '0';  /* get next digit */
   } while ((n /= 10) > 0);   /* delete it */

	 // reverse
	 for(j=0;j<i/2;++j) {
		 char t = s[j];
		 s[j] = s[i-j];
		 s[i-j] = t;
	 }
}
