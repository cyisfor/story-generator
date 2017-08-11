const char* db_oid_str(db_oid oid) {
	return git_oid_tostr_s((git_oid*)oid);
}

git_oid* GIT_OID(db_oid a) {
	return (git_oid*)a;
}
