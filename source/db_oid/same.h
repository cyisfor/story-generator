const char* db_oid_str(db_oid oid) {
	return git_oid_strp(*((git_oid*)oid));
}
