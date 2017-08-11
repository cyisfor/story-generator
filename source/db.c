#include "db.h"
#include "ensure.h"

#include <sqlite3.h>
#include <error.h>
#include <assert.h>
#include <string.h> // memcpy

#include "db_oid.gen.c"

sqlite3* db = NULL;

static int db_check(int res) {
	switch(res) {
	case SQLITE_OK:
	case SQLITE_DONE:
	case SQLITE_ROW:
		return res;
	};

	if(!db) {
		error(0,0,"no db %d %s",res, sqlite3_errstr(res));
		abort();
	}
	error(0,0,"db error %d(%s) %s",
				res,sqlite3_errstr(res),
				sqlite3_errmsg(db));
	abort();
	return res;
}

void db_open(const char* filename) {
	db_check(sqlite3_open(filename,&db));
	assert(db != NULL);
#include "db-sql.gen.c"
	char* err = NULL;
	db_check(sqlite3_exec(db, sql, NULL, NULL, &err));
}


/* since sqlite sucks, have to malloc copies of pointers to all statements,
	 since the database won't close until we finalize them all. */
sqlite3_stmt** stmts = NULL;
size_t nstmt = 0;
static void add_stmt(sqlite3_stmt* stmt) {
	stmts = realloc(stmts,sizeof(*stmts)*(++nstmt));
	stmts[nstmt-1] = stmt;
}

#define PREPARE(stmt,sql) {																	 \
	db_check(sqlite3_prepare_v2(db, LITLEN(sql), &stmt, NULL)); \
	add_stmt(stmt);																							 \
	}

#define DECLARE_STMT(stmt,sql)																		\
	static sqlite3_stmt* stmt = NULL;																\
	if(stmt == NULL) {																							\
		PREPARE(stmt, sql);																						\
	}

#define DECLARE_DB_FUNC(name,sql) static void name(void) { \
	DECLARE_STMT(stmt, sql);																 \
	db_once(stmt);																					 \
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

#define BEGIN_TRANSACTION(name) void intrans ## name(void) {
#define END_TRANSACTION(name) }; db_transaction(intrans ## name);

static void db_once_trans(sqlite3_stmt* stmt) {
	// because sqlite is retarded, no changes take effect outside a transaction
	void intrans(void) {
		db_once_intrans(stmt);
	}
	db_transaction(intrans);
}

DECLARE_DB_FUNC(begin, "BEGIN");
DECLARE_DB_FUNC(commit, "COMMIT");
DECLARE_DB_FUNC(rollback, "ROLLBACK");

enum commit_kind {
	PENDING, /* the soonest commit we've seen, but not finished all before */
	LAST, /* the soonest commit we've finished with */
	CURRENT /* the oldest commit we've seen, but not finished with */
};

bool saw_commit = false;

void db_saw_commit(git_time_t timestamp, db_oid commit) {
	DECLARE_STMT(insert,"INSERT OR IGNORE INTO commits (oid,timestamp,kind) VALUES (?,?,?)\n");
	DECLARE_STMT(delete,"DELETE FROM commits WHERE kind = ?\n");
	BEGIN_TRANSACTION(saw);
	sqlite3_bind_int(delete, 1, CURRENT);
	db_once(delete);
	sqlite3_bind_blob(insert, 1, commit, sizeof(db_oid), NULL);
	sqlite3_bind_int(insert, 2, timestamp);
	sqlite3_bind_int(insert, 3, CURRENT);
	db_once(insert);
	if(saw_commit) return;
	saw_commit = true;
	sqlite3_bind_int(insert,3,PENDING);
	db_once(insert);
	END_TRANSACTION(saw);
}

void db_caught_up(void) {
	if(!saw_commit) return;
	void intrans(void) {
	DECLARE_STMT(delete,"DELETE FROM last_commit");
	DECLARE_STMT(commit,"INSERT INTO last_commit SELECT oid,timestamp FROM pending_commit");
	DECLARE_STMT(drop,"DELETE FROM pending_commit");
	db_once(delete);
	db_once(commit);
	db_once(drop);
	}
	db_transaction(intrans);
}

bool db_last_seen_commit(db_oid commit, git_time_t* timestamp) {
	DECLARE_STMT(find,"SELECT oid,timestamp FROM last_commit");

	int res = sqlite3_step(find);
	switch(res) {
	case SQLITE_DONE:
		sqlite3_reset(find);
		return false;
	case SQLITE_ROW:
		assert(sizeof(db_oid) == sqlite3_column_bytes(find,0));
		const char* blob = sqlite3_column_blob(find, 0);
		assert(blob != NULL);
		memcpy(commit, blob, sizeof(db_oid));
		*timestamp = sqlite3_column_int64(find,1);
		sqlite3_reset(find);
		return true;
	default:
		db_check(res);
		abort();
	};
}

identifier db_find_story(const string location, git_time_t timestamp) {
	DECLARE_STMT(find,"SELECT id FROM stories WHERE location = ?");
	DECLARE_STMT(insert,"INSERT INTO stories (location,timestamp) VALUES (?,?)");
	DECLARE_STMT(update,"UPDATE stories SET timestamp = MAX(timestamp,?) WHERE id = ?");

	identifier id = 0;
	void intrans(void) {
	sqlite3_bind_blob(find,1,location.s,location.l,NULL);
	int res = db_check(sqlite3_step(find));
	if(res == SQLITE_ROW) {
		id = sqlite3_column_int64(find,0);
		sqlite3_reset(find);
		sqlite3_bind_int64(update,1,timestamp);
		sqlite3_bind_int64(update,2,id);
		db_once(update);
		return;
	} else {
		sqlite3_reset(find);
		sqlite3_bind_blob(insert,1,location.s, location.l, NULL);
		sqlite3_bind_int64(insert,2,timestamp);
		db_once(insert);
		id = sqlite3_last_insert_rowid(db);
		return;
	}
	}
	db_transaction(intrans);
	return id;
}

void db_saw_chapter(bool deleted, identifier story,
										git_time_t timestamp, identifier chapter) {
	if(deleted) {
		DECLARE_STMT(delete, "DELETE FROM chapters WHERE story = ? AND chapter = ?");
		sqlite3_bind_int64(delete,1,story);
		sqlite3_bind_int64(delete,2,chapter);
		db_once(delete);
	} else {
		DECLARE_STMT(update,"UPDATE chapters SET timestamp = ? WHERE story = ? AND chapter = ?"); 
		DECLARE_STMT(insert,"INSERT INTO chapters (timestamp,story,chapter) VALUES (?,?,?)");
		sqlite3_bind_int64(update,1,timestamp);
		sqlite3_bind_int64(update,2,story);
		sqlite3_bind_int64(update,3,chapter);
		db_once(update);
		if(sqlite3_changes(db) > 0) return;
		sqlite3_bind_int64(insert,1,timestamp);
		sqlite3_bind_int64(insert,2,story);
		sqlite3_bind_int64(insert,3,chapter);
		db_once(insert);
	}
}


void db_for_stories(void (*handle)(identifier story,
																	 const string location,
																	 bool finished,
																	 size_t numchaps,
																	 git_time_t timestamp),
										git_time_t since) {
	DECLARE_STMT(find,"SELECT id,location,finished,(SELECT COUNT(chapter) FROM chapters WHERE story = stories.id),timestamp FROM stories WHERE timestamp AND timestamp > ?");
	sqlite3_bind_int64(find,1,since);
	for(;;) {
		int res = sqlite3_step(find);
		switch(res) {
		case SQLITE_ROW: {
			const string location = {
				.s = sqlite3_column_blob(find,1),
				.l = sqlite3_column_bytes(find,1)
			};
			handle(sqlite3_column_int64(find,0),
						 location,
						 sqlite3_column_int(find,2) == 1,
						 sqlite3_column_int64(find,3),
						 sqlite3_column_int64(find,4));
			continue;
		}
		case SQLITE_DONE:
			sqlite3_reset(find);
			return;
		default:
			db_check(res);
			abort();
		};
	}
}

void db_for_chapters(identifier story,
										void (*handle)(identifier chapter,
																	 git_time_t timestamp),
										git_time_t since) {
	DECLARE_STMT(find,
							 "SELECT chapter,timestamp FROM chapters WHERE story = ? AND timestamp > ?");
	sqlite3_bind_int64(find,1,story);
	sqlite3_bind_int64(find,2,since);
	for(;;) {
		int res = sqlite3_step(find);
		switch(res) {
		case SQLITE_ROW:
			handle(sqlite3_column_int64(find,0),
						 sqlite3_column_int64(find,1));
			continue;
		case SQLITE_DONE:
			sqlite3_reset(find);
			return;
		default:
			db_check(res);
			abort();
		};
	}
}

void db_with_chapter_title(identifier story,
													 identifier chapter,
													 void (*handle)(const string)) {
	DECLARE_STMT(find,"SELECT title FROM chapters WHERE story = ? AND chapter = ?");
	sqlite3_bind_int64(find,1,story);
	sqlite3_bind_int64(find,2,chapter);
	int res = sqlite3_step(find);
	if(res == SQLITE_ROW) {
		const string title = {
			.s = sqlite3_column_blob(find,0),
			.l = sqlite3_column_bytes(find,0)
		};
		handle(title);
	} else {
		db_check(res);
		string title = {};
		handle(title);
	}
	sqlite3_reset(find);
}

bool derp = false;
void db_with_story_info(const identifier story, void (*handle)(string title,
																												 string description,
																												 string source)) {
	assert(derp == false); // not reentrant!
	derp = true;
	DECLARE_STMT(find,"SELECT title,description,source FROM stories WHERE id = ? AND ("
		"title IS NOT NULL OR "
		"description IS NOT NULL OR "
		"source IS NOT NULL)");
	sqlite3_bind_int64(find,1,story);
	string title = {};
	string description = {};
	string source = {};
	printf("start find %d\n",story);
	int res = sqlite3_step(find);
	if(res == SQLITE_ROW) {
		void CHECK(int col, string* str) {
			str->s = sqlite3_column_blob(find,col); 
			str->l = sqlite3_column_bytes(find,col);
		}
		CHECK(0,&title);
		CHECK(1,&description);
		CHECK(2,&source);
	} else {
		db_check(res);
	}
	// handle if nothing in the db, so we can search for files and stuff.
	// XXX: TODO: only call a special creation function if not found?
	handle(title,description,source);
	printf("reset find %d\n",story);
	sqlite3_reset(find);
	derp = false;
}

// should set to NULL if string is empty
void db_set_chapter_title(const string title, identifier story, identifier chapter) {
	DECLARE_STMT(update,"UPDATE chapters SET title = ? WHERE story = ? AND chapter = ?");
	sqlite3_bind_blob(update,1,title.s,title.l,NULL);
	sqlite3_bind_int64(update,2,story);
	sqlite3_bind_int64(update,3,chapter);
	db_once(update);
}
void db_set_story_info(identifier story,
											 const string title,
											 const string description,
											 const string source) {
	DECLARE_STMT(update,"UPDATE stories SET title = COALESCE(?,title),"
							 "description = COALESCE(?,description),"
							 "source = COALESCE(?,source) "
							 "WHERE id = ?");
	void one(int col, const string thing) {
		if(thing.l == 0 || thing.s == NULL) {
			sqlite3_bind_null(update,col);
		} else {
			sqlite3_bind_blob(update,col,thing.s,thing.l,NULL);
		}
	}
	one(1,title);
	one(2,description);
	one(3,source);
	sqlite3_bind_int64(update,4,story);
	db_once(update);
}

void db_transaction(void (*run)(void)) {
	static bool in_trans = false;

	if(in_trans) return run();
	in_trans = true;
	begin();
	run();
	commit();
	in_trans = false;
}

void db_retransaction(void) {
	commit();
	begin();
}
