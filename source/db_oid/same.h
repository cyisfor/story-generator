const char* db_oid_str(db_oid oid) {
	return git_oid_tostr_s((git_oid*)oid);
}
