#define DB_OID(o) o.id
typedef unsigned char db_oid[GIT_OID_RAWSZ]; // to .h

void db_saw_commit(git_time_t timestamp, db_oid commit) {
	static sqlite3_stmt *insert = NULL;
	if(insert == NULL) {
		PREPARE(insert,"INSERT OR REPLACE INTO commits (oid,timestamp) VALUES (?,?)");
	}
	sqlite3_bind_blob(insert, 1, commit, sizeof(commit), NULL);
	sqlite3_bind_int(insert, 2, timestamp);
	db_once(insert);
}

bool db_last_seen_commit(db_oid commit) {
	static sqlite3_stmt *find = NULL;
	if(find == NULL) {
		PREPARE(find,"SELECT oid FROM commits ORDER BY timestamp DESC LIMIT 1");
	}
	int res = sqlite3_step(find);
	switch(res) {
	case SQLITE_DONE:
		sqlite3_reset(find);
		return false;
	case SQLITE_ROW:
		assert(sizeof(git_oid) == sqlite3_column_bytes(find,0));
		ensure0(memcpy(oid, sqlite3_column_blob(find, 0), len));
		sqlite3_reset(find);
		return true;
	default:
		db_check(res);
		abort();
	};
}
	
	
