#define PREPARE(name,stmt) {																	 \
	db_check(sqlite3_prepare_v2(db, LITLEN(stmt), &name, NULL)); \
	add_stmt(name);																							 \
	}

#define DECLARE_STMT(name,stmt)																		\
	static sqlite3_stmt* name = NULL;																\
	if(name == NULL) {																							\
		PREPARE(name, stmt);																					\
	}

sqlite3* db = NULL;

static void db_check(int res) {
	switch(res) {
	case SQLITE_OK:
	case SQLITE_DONE:
	case SQLITE_ROW:
		return;
	};
	
	if(!db) {
		error(23,0,"no db %d %s",res, sqlite3_errstr(res));
	}
	error(42,0,"db error %d(%s) %s",
				res,sqlite_errstr(res),
				sqlite3_errmsg(db));
}

void db_open(const char* filename) {
	db_check(sqlite3_open(filename,&db));
	assert(db != NULL);
}

/* since sqlite sucks, have to malloc copies of pointers to all statements,
	 since the database won't close until we finalize them all. */
sqlite3_statement** stmts = NULL;
size_t nstmt = 0;
static void add_stmt(sqlite3_stmt* stmt) {
	stmts = realloc(stmts,++nstmt);
	stmts[nstmt-1] = stmt;
}

static sqlite3_stmt* begin(void) {
	DECLARE_STMT(stmt,"BEGIN");
	return stmt;
}
static sqlite3_stmt* commit(void) {
	DECLARE_STMT(stmt,"COMMIT");
	return stmt;
}
static sqlite3_stmt* rollback(void) {
	DECLARE_STMT(stmt,"ROLLBACK");
	return stmt;
}

void db_close(void) {
	size_t i;
	for(i=0;i<nstmt;++i) {
		db_check(sqlite3_finalize(stmts[i]));

#define DB_OID(o) o.id
typedef unsigned char db_oid[GIT_OID_RAWSZ]; // to .h

void db_saw_commit(git_time_t timestamp, db_oid commit) {
	DECLARE_STMT(insert,"INSERT OR REPLACE INTO commits (oid,timestamp) VALUES (?,?)");
	sqlite3_bind_blob(insert, 1, commit, sizeof(commit), NULL);
	sqlite3_bind_int(insert, 2, timestamp);
	db_once(insert);
}


bool db_last_seen_commit(db_oid commit) {
	DECLARE_STMT(find,"SELECT oid FROM commits ORDER BY timestamp DESC LIMIT 1");
	
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
	
void db_saw_chapter(bool deleted, string location, long int chapnum) {
	identifier story = db_find_story(location);
	if(deleted) {
		DECLARE_STMT(delete, "DELETE FROM chapters WHERE story = ? AND chapter = ?");
		sqlite3_bind_int64(delete,1,story);
		sqlite3_bind_int64(delete,2,chapnum);
		db_once(delete);
	} else {
		DECLARE_STMT(insert, "INSERT INTO chapters WHERE story = ? AND chapter = ?");
		sqlite3_bind_int64(insert,1,story);
		sqlite3_bind_int64(insert,2,chapnum);
		db_once(insert);
	}
