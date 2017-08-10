#include "db.h"

sqlite3* db = NULL;

static int db_check(int res) {
	switch(res) {
	case SQLITE_OK:
	case SQLITE_DONE:
	case SQLITE_ROW:
		return res;
	};

	if(!db) {
		error(23,0,"no db %d %s",res, sqlite3_errstr(res));
	}
	error(42,0,"db error %d(%s) %s",
				res,sqlite_errstr(res),
				sqlite3_errmsg(db));
	return res;
}

void db_open(const char* filename) {
	db_check(sqlite3_open(filename,&db));
	assert(db != NULL);
#include "db-sql.gen.c"
	const char* err = NULL;
	db_check(sqlite3_exec(db, sql, NULL, NULL, &err));
}

void db_close_and_exit(void) {
	size_t i;
	for(i=0;i<nstmt;++i) {
		db_check(sqlite3_finalize(stmts[i]));
	}
	db_check(sqlite3_close(db));
	/* if we do any DB thing after this, it will die horribly.
		 if we need to continue past this, and reopen the db, have to:
		 1) preserve a copy of each SQL statement, before finalizing, via sqlite3_expanded_sql
		 2) when reopening, iterate to nstmt, re-preparing each one, then freeing the SQL string
		 3) change all use of statements to be stmts[stmt] where stmt is an index into stmts
	*/
	exit(0);
}

static void db_once(sqlite3_stmt* stmt) {
	int res = sqlite3_step(stmt);
	sqlite3_reset(stmt);
	db_check(res);
}

/* since sqlite sucks, have to malloc copies of pointers to all statements,
	 since the database won't close until we finalize them all. */
sqlite3_statement** stmts = NULL;
size_t nstmt = 0;
static void add_stmt(sqlite3_stmt* stmt) {
	stmts = realloc(stmts,++nstmt);
	stmts[nstmt-1] = stmt;
}

#define PREPARE(name,stmt) {																	 \
	db_check(sqlite3_prepare_v2(db, LITLEN(stmt), &name, NULL)); \
	add_stmt(name);																							 \
	}

#define DECLARE_STMT(name,stmt)																		\
	static sqlite3_stmt* name = NULL;																\
	if(name == NULL) {																							\
		PREPARE(name, stmt);																					\
	}

#define DECLARE_DB_FUNC(name,sql) static void name(void) { \
	DECLARE_STMT(stmt, sql);																 \
	db_once(stmt);																					 \
	}

DECLARE_DB_FUNC(begin, "BEGIN");
DECLARE_DB_FUNC(commit, "COMMIT");
DECLARE_DB_FUNC(rollback, "ROLLBACK");

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

identifier db_find_story(const string location) {
	DECLARE_STMT(find,"SELECT id FROM STORIES WHERE location = ?");
	DECLARE_STMT(insert,"INSERT OR IGNORE INTO STORIES (location) VALUES (?)");

	begin();
	sqlite3_bind_blob(find,1,location.s,location.l,NULL);
	int res = db_check(sqlite3_step(find));
	if(res == SQLITE_ROW) {
		identifier id = sqlite3_column_int64(find,0);
		sqlite3_reset(find);
		return id;
	} else {
		sqlite3_reset(find);
		sqlite3_bind_blob(insert,1,location.s, location.l);
		sqlite3_bind_int64(insert,2,timestamp);
		db_once(insert);
		return sqlite3_last_insert_rowid(db);
	}
}

void db_saw_chapter(bool deleted, identifier story, git_time_t timestamp, long int chapnum) {
	if(deleted) {
		DECLARE_STMT(delete, "DELETE FROM chapters WHERE story = ? AND chapter = ?");
		sqlite3_bind_int64(delete,1,story);
		sqlite3_bind_int64(delete,2,chapnum);
		db_once(delete);
	} else {
		DECLARE_STMT(insert, "INSERT OR REPLACE INTO chapters (story,chapter,timestamp) VALUES (?,?,?)");
		sqlite3_bind_int64(insert,1,story);
		sqlite3_bind_int64(insert,2,chapnum);
		sqlite3_bind_int64(insert,3,timestamp);
		db_once(insert);
	}
}
