const char* db_oid_str(const db_oid oid) {
	static char buf[(sizeof(db_oid)<<1)+1] = "";
	//static char digits[] = "QBPVFZSDTJCGKYXW"; wonk
	static char digits[] = "0123456789abcdef"; 
	assert(sizeof(digits) == 0x11);
	int i;
	for(i=0;i<sizeof(db_oid);++i) {
		char lo = oid[i] & 0xF;
		char hi = (oid[i]>>4) & 0xF;
		assert(lo < 0x10);
		assert(hi < 0x10);
		buf[i<<1] = digits[hi];
		buf[(i<<1)+1] = digits[lo];
	}
	return buf;
}

const git_oid* GIT_OID(const db_oid a) {
	static git_oid temp; // ugh
	memcpy(temp.id,a,sizeof(db_oid));
	return &temp;
}
