const char* db_oid_str(const db_oid oid) {
	return git_oid_tostr_s((const git_oid*)oid);
}

const git_oid* GIT_OID(const db_oid a) {
	return (git_oid*)a;
}
