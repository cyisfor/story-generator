// how about two lists, a buffer and a main?
/* to search, first you bsearch main, and if not, then buffer (or vice versa)
	 to insert, shift smaller mem of buffer to make room, until threshold
	 then sort it all at once into main

	 a b c d e f  / c1 e1 g1
	 insert d1
	 check c, too low, e too high, d too high, done.
	 check c1 too low, e1 too high, done
	 shift e1/g1 right, to put d1 in there.

	 a b c d e f / c1 _ e1 g1
	 a b c d e f / c1 d1 e1 g1

	 inc. counter, now we hit threshold of 4. expand list by 4:
	 a b c d e f c1 d1 e1 g1 _ _ _ _
	 now mergesort to g1.

	 meh, just mergesort every time.
 */
