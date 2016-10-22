// sigh

#include "ddate.h"

const char* current_ddate(time_t now) {
	if(now == -1) {
		now = time(NULL);
	}
	static char buf[0x100];
	int amt = disc_format(buf,0x100,NULL,disc_fromtm(gmtime(&now)));
	buf[amt] = '\0';
	return buf;
}
