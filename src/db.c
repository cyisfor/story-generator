#include "db.h"
#include "ensure.h"

#include <sqlite3.h>
#include <error.h>
#include <assert.h>
#include <string.h> // memcpy

#include "db_oid/gen.h"

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


static int db_once(sqlite3_stmt* stmt) {
		int res = sqlite3_step(stmt);
		sqlite3_reset(stmt);
		return db_check(res);
}

/* A transaction level is VERY important for speed
	 so you can have "safe" operations that will make
	 their own transaction, but then when wrapping them
	 in a transaction, they won't commit early, and be slow
*/

static int transaction_level = 0;

#define DECLARE_BUILTIN(name)										\
	sqlite3_stmt *name ## _stmt = NULL;						\
	void db_ ## name(void)

DECLARE_BUILTIN(begin) {
	if(++transaction_level != 1) return;
	//SPAM("begin");

	db_once(begin_stmt);
}
DECLARE_BUILTIN(commit) {
	ensure_ne(transaction_level,0);
	if(--transaction_level != 0) return;
//	SPAM("commit");
	db_once(commit_stmt);
}
DECLARE_BUILTIN(rollback) {
	ensure_ne(transaction_level,0);
	if(--transaction_level != 0) return;
	db_once(rollback_stmt);
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
	assert(stmt != NULL);																				 \
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


struct {
	bool ye;
	identifier i;
} only_story = {};

void db_open(const char* filename) {
	db_check(sqlite3_open(filename,&db));
	assert(db != NULL);
#include "db-sql.gen.c"
	PREPARE(begin_stmt,"BEGIN");
	PREPARE(commit_stmt,"COMMIT");
	PREPARE(rollback_stmt,"ROLLBACK");
	char* err = NULL;
	BEGIN_TRANSACTION;
	db_check(sqlite3_exec(db, sql, NULL, NULL, &err));
	END_TRANSACTION;

	string s = { .s = getenv("story") };
	if(s.s != NULL) {
		s.l = strlen(s.s);
		only_story.i = db_find_story(s);
		ensure_ge(only_story.i,0);
		only_story.ye = true;
	}
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

static int db_once_trans(sqlite3_stmt* stmt) {
	// because sqlite is retarded, no changes take effect outside a transaction
	BEGIN_TRANSACTION;
	int ret = db_once(stmt);
	END_TRANSACTION;
	return ret;
}

enum commit_kind {
	PENDING, /* the soonest commit we've seen, but not finished all before */
	LAST, /* the soonest commit we've finished with */
	CURRENT /* the oldest commit we've seen, but not finished with */
};

bool saw_commit = false;

void db_saw_commit(git_time_t timestamp, db_oid commit) {
	static sqlite3_stmt* insert_current = NULL, *insert_pending;
	if(insert_current == NULL) {
		
		PREPARE(insert_current,
						"INSERT OR REPLACE INTO commits (kind,oid,timestamp) \n"
						"VALUES (?,?,?)");
		// Note: insert pending uses what was inserted for insert_current, but changes kind
		PREPARE(insert_pending,
						"INSERT OR IGNORE INTO commits (oid,timestamp,kind) \n"
						"SELECT oid,timestamp,? FROM commits "
						"WHERE kind = ?");
		// these never change
		sqlite3_bind_int(insert_current, 1, CURRENT);
		sqlite3_bind_int(insert_pending, 1, PENDING);
		sqlite3_bind_int(insert_pending, 2, CURRENT);
	}
	sqlite3_bind_blob(insert_current, 2, commit, sizeof(db_oid), NULL);
	sqlite3_bind_int(insert_current, 3, timestamp);
	BEGIN_TRANSACTION;
	db_once(insert_current);
	if(!saw_commit) {
		saw_commit = true;
		db_once(insert_pending);
	}
	END_TRANSACTION;
}


void db_caught_up_commits(void) {
	static sqlite3_stmt* update = NULL, *nocurrent;
	if(update == NULL) {
		PREPARE(update,"UPDATE OR REPLACE commits SET kind = ? WHERE kind = ?");
		PREPARE(nocurrent,"DELETE FROM commits WHERE kind = ?");
		// these never change
		sqlite3_bind_int(update,1,LAST);
		sqlite3_bind_int(update,2,PENDING);
		sqlite3_bind_int(nocurrent,1,CURRENT);
	}
	BEGIN_TRANSACTION;
	db_once(nocurrent);
	if(saw_commit)
		db_once(update);
	END_TRANSACTION;
}


identifier db_get_category(const string name, git_time_t* timestamp) {
	DECLARE_STMT(find,"SELECT id,timestamp FROM categories WHERE category = ?");
	DECLARE_STMT(insert,"INSERT INTO categories (category) VALUES(?)");
	sqlite3_bind_text(find,1,name.s,name.l,NULL);
	RESETTING(find) int res = sqlite3_step(find);
	TRANSACTION; 
	if(res == SQLITE_ROW) {
		*timestamp = sqlite3_column_int64(find,1);
		return sqlite3_column_int64(find,0);
	}
	*timestamp = 0;
	sqlite3_bind_text(insert,1,name.s,name.l,NULL);
	db_once(insert);
	identifier ret = sqlite3_last_insert_rowid(db);
	return ret;
}

void db_last_seen_commit(struct bad* out,
												 db_oid last, db_oid current) {
	static sqlite3_stmt* find = NULL;
	if(find == NULL) {
		PREPARE(find,"SELECT oid,timestamp FROM commits WHERE kind = ?");
	}

	bool one(db_oid dest, enum commit_kind kind) {
		sqlite3_bind_int(find,1,kind);
		RESETTING(find) int res = sqlite3_step(find);
		switch(res) {
		case SQLITE_DONE:
			return false;
		case SQLITE_ROW:
			assert(sizeof(db_oid) == sqlite3_column_bytes(find,0));
			const char* blob = sqlite3_column_blob(find, 0);
			assert(blob != NULL);
			memcpy(dest, blob, sizeof(db_oid));
			return true;
		default:
			db_check(res);
			abort();
		};
	}
	out->current = one(current,CURRENT);
	out->last = one(last,LAST);
}



void db_caught_up_category(identifier category) {
	DECLARE_STMT(update,"UPDATE categories SET timestamp = ? WHERE id = ?");

	sqlite3_bind_int64(update,1,time(NULL));
	sqlite3_bind_int64(update,2,category);
	db_once_trans(update);
}
	
identifier db_find_story(const string location) {
	DECLARE_STMT(find,"SELECT id FROM stories WHERE location = ?");
	sqlite3_bind_text(find,1,location.s,location.l,NULL);
	RESETTING(find) int res = db_check(sqlite3_step(find));
	if(res == SQLITE_ROW) {
		return sqlite3_column_int64(find,0);
	}
	return -1;
}

identifier db_get_story(const string location, git_time_t timestamp) {
	DECLARE_STMT(find,"SELECT id FROM stories WHERE location = ?");
	DECLARE_STMT(insert,"INSERT INTO stories (location,timestamp) VALUES (?,?)");
	DECLARE_STMT(update,"UPDATE stories SET timestamp = MAX(timestamp,?) WHERE id = ?");

	sqlite3_bind_text(find,1,location.s,location.l,NULL);
	TRANSACTION;
	RESETTING(find) int res = db_check(sqlite3_step(find));
	if(res == SQLITE_ROW) {
		identifier id = sqlite3_column_int64(find,0);
		sqlite3_bind_int64(update,1,timestamp);
		sqlite3_bind_int64(update,2,id);
		db_once_trans(update);
		return id;
	} else {
		sqlite3_bind_text(insert,1,location.s, location.l, NULL);
		sqlite3_bind_int64(insert,2,timestamp);
		db_once(insert);
		return sqlite3_last_insert_rowid(db);
	}
}

// how low can we stoop?
bool need_restart_for_chapters = false;

void db_saw_chapter(bool deleted, identifier story,
										git_time_t timestamp, identifier chapter) {
	if(deleted) {
		DECLARE_STMT(delete, "DELETE FROM chapters WHERE story = ? AND chapter = ?");
		sqlite3_bind_int64(delete,1,story);
		sqlite3_bind_int64(delete,2,chapter);
		db_once_trans(delete);
	} else {
		INFO("SAW %d:%d",story,chapter);
		DECLARE_STMT(find,"SELECT timestamp FROM chapters WHERE story = ? AND chapter = ?");
		DECLARE_STMT(update,"UPDATE chapters SET timestamp = MAX(timestamp,?) "
								 "WHERE story = ? AND chapter = ?"); 
		DECLARE_STMT(insert,"INSERT INTO chapters (timestamp,story,chapter) VALUES (?,?,?)");
		sqlite3_bind_int64(find,1,story);
		sqlite3_bind_int64(find,2,chapter);
		TRANSACTION;
		RESETTING(find) int res = sqlite3_step(find);
		switch(res) {
		case SQLITE_ROW:
			// update if new timestamp is higher
			if(sqlite3_column_int64(find,0) < timestamp) {
				sqlite3_bind_int64(update,1,timestamp);
				sqlite3_bind_int64(update,2,story);
				sqlite3_bind_int64(update,3,chapter);
				db_once(update);
				assert(sqlite3_changes(db) > 0);
				// any for_chapters iterator has to be restarted now.
				need_restart_for_chapters = true;
			}
			return;
		case SQLITE_DONE:
			sqlite3_bind_int64(insert,1,timestamp);
			sqlite3_bind_int64(insert,2,story);
			sqlite3_bind_int64(insert,3,chapter);
			db_once(insert);
			return;
		};
	}
}

/* be CAREFUL none of these iterators are re-entrant! */

void db_for_recent_chapters(int limit,
														void (*handle)(identifier story,
																					 size_t chapnum,
																					 const string title,
																					 const string location,
																					 git_time_t timestamp)) {

	DECLARE_STMT(find,"SELECT story,chapter,"
							 "(select title from stories where stories.id = story),"
							 "(select location from stories where stories.id = story),"
							 "timestamp FROM chapters "
/*							 "WHERE "
							 "(select finished from stories where stories.id = story) OR "
							 "chapter < (select count(1) from chapters as sub where sub.story = chapters.story) "*/
							 "ORDER BY timestamp DESC LIMIT ?");
	RESETTING(find) int res;

	sqlite3_bind_int(find,1,limit);

	for(;;) {
		res = db_check(sqlite3_step(find));
		switch(res) {
		case SQLITE_ROW: {
			const string title = {
				.s = sqlite3_column_blob(find,2),
				.l = sqlite3_column_bytes(find,2)
			};
			const string location = {
				.s = sqlite3_column_blob(find,3),
				.l = sqlite3_column_bytes(find,3)
			};
			handle(
				sqlite3_column_int64(find,0),
				sqlite3_column_int64(find,1),
				title,
				location,
				sqlite3_column_int64(find,4));
			continue;
		}
		case SQLITE_DONE:
			return;
		};
	}
}

void db_for_stories(void (*handle)(identifier story,
																	 const string location,
																	 bool finished,
																	 size_t numchaps,
																	 git_time_t timestamp),
										git_time_t since) {
	if(only_story.ye) {
		DECLARE_STMT(find,"SELECT location,finished,chapters,timestamp FROM stories WHERE id = ?");
		sqlite3_bind_int64(find,1,only_story.i);
		RESETTING(find) int res = db_check(sqlite3_step(find));
		ensure_eq(res,SQLITE_ROW);
		const string location = {
			.s = sqlite3_column_blob(find,0),
			.l = sqlite3_column_bytes(find,0)
		};
		handle(only_story.i,
					 location,
					 sqlite3_column_int(find,1) == 1,
					 sqlite3_column_int64(find,2),
					 sqlite3_column_int64(find,3));
		return;
	}

	DECLARE_STMT(find,"SELECT id,location,finished,chapters,timestamp FROM stories WHERE timestamp AND timestamp > ? ORDER BY timestamp");

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

void db_for_undescribed_stories(void (*handle)(identifier story,
																							 const string title,
																							 const string description,
																							 const string source)) {
	DECLARE_STMT(find,"SELECT id,coalesce(title,location),description,source FROM stories WHERE "
							 "title IS NULL OR title = '' OR "
							 "description IS NULL OR description = ''");
	for(;;) {
		RESETTING(find) int res = db_check(sqlite3_step(find));
		if(res == SQLITE_DONE) return;
		ensure_eq(res,SQLITE_ROW);
		identifier story = sqlite3_column_int64(find,0);
		string title = {
			.s = sqlite3_column_blob(find,1),
			.l = sqlite3_column_bytes(find,1)
		};
		string description = {
			.s = sqlite3_column_blob(find,2),
			.l = sqlite3_column_bytes(find,2)
		};
		string source = {
			.s = sqlite3_column_blob(find,3),
			.l = sqlite3_column_bytes(find,3)
		};
		handle(story,title,description,source);
	}	
}


void db_for_chapters(identifier story,
										void (*handle)(identifier chapter,
																	 git_time_t timestamp),
										git_time_t since) {
	DECLARE_STMT(find,
							 "SELECT chapter,timestamp FROM chapters WHERE story = ? AND timestamp > ? ORDER BY timestamp");
RESTART:
	sqlite3_bind_int64(find,1,story);
	sqlite3_bind_int64(find,2,since);
	for(;;) {
		int res = sqlite3_step(find);
		switch(res) {
		case SQLITE_ROW:
			handle(sqlite3_column_int64(find,0),
						 sqlite3_column_int64(find,1));
			if(need_restart_for_chapters) {
				/* okay, so since in order of ascending timestamp...
					 we can re-bind a new timestamp to find,
					 only checking (again) from now onward
					 no chapter should be added with an OLDER timestamp
					 than the current one!
				*/
				since = sqlite3_column_int64(find,1);
				sqlite3_bind_int64(find,2,since);
				sqlite3_reset(find);
				need_restart_for_chapters = false;
			}
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
	RESETTING(find) int res = sqlite3_step(find);
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
	RESETTING(find) int res = sqlite3_step(find);
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
	derp = false;
}

identifier db_count_chapters(identifier story) {
	DECLARE_STMT(find,"SELECT COUNT(chapter) FROM chapters WHERE story = ?");
	sqlite3_bind_int64(find,1,story);
	RESETTING(find) int res = sqlite3_step(find);
	if(res == SQLITE_ROW) {
		identifier count = sqlite3_column_int64(find,0);
		return count;
	}
	db_check(res);
	// COUNT(...) always returns rows!
	abort();
}

// should set to NULL if string is empty
void db_set_chapter_title(const string title,
													identifier story, identifier chapter,
													bool* title_changed) {
	DECLARE_STMT(update,"UPDATE chapters SET title = ? WHERE story = ? AND chapter = ?");
	sqlite3_bind_text(update,1,title.s,title.l,NULL);
	sqlite3_bind_int64(update,2,story);
	sqlite3_bind_int64(update,3,chapter);
	BEGIN_TRANSACTION;
	db_once(update);
	if(!*title_changed) {
		if(sqlite3_changes(db) > 0) *title_changed = true;
		// never set title_changed to false, it goes true it stays true
	}
	END_TRANSACTION;
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
			sqlite3_bind_text(update,col,thing.s,thing.l,NULL);
		}
	}
	one(1,title);
	one(2,description);
	one(3,source);
	sqlite3_bind_int64(update,4,story);
	db_once_trans(update);
}

void db_set_chapters(identifier story, size_t numchaps) {
	DECLARE_STMT(update,"UPDATE stories SET chapters = ? "
							 "WHERE id = ?");
	sqlite3_bind_int64(update,1,numchaps);
	sqlite3_bind_int64(update,2,story);
	db_once_trans(update);
}

void db_transaction(void (*run)(void)) {
	if(transaction_level > 0) return run();
	db_once(begin_stmt);
	transaction_level = 1;
	run();
	ensure_eq(transaction_level, 1);
	db_once(commit_stmt);
	transaction_level = 0;
}

void db_retransaction(void) {
	SPAM("retransaction %d",transaction_level);
	if(transaction_level == 0) {
		db_once(begin_stmt);
		transaction_level = 1;
	} else {
		// NOT db_commit
		db_once(commit_stmt);
		db_once(begin_stmt);
	}
}


// this is SUCH a hack
static bool is_cool_xml_tag(const char* tag, size_t tlen) {
	if(!db) return true;
	DECLARE_STMT(find,"SELECT 1 FROM cool_xml_tags WHERE tag = ?");
	sqlite3_bind_text(find,1,tag,tlen,NULL);
	int res = sqlite3_step(find);
	sqlite3_reset(find);
	db_check(res);
	return res == SQLITE_ROW ? 1 : 0;
}

void cool_xml_error_handler(void * userData, xmlErrorPtr error) {
	if(error->code == XML_HTML_UNKNOWN_TAG) {
		const char* name = error->str1;
		size_t nlen = strlen(name);
		if(is_cool_xml_tag(name,nlen)) return;
	}
	fprintf(stderr,"xml error %s %s\n",error->message,
					error->level == XML_ERR_FATAL ? "fatal..." : "ok");
}

	
