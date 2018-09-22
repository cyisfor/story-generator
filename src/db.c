#include "note.h"
#include "db.h"
#include "ensure.h"
#include "htmlish.h"

#include <sqlite3.h>
#include <error.h>
#include <assert.h>
#include <string.h> // memcpy

#include "db_oid/gen.h"

#include "db-private.h"
sqlite3* db = NULL;

int db_check(int res) {
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
	error(0,0,"db error %d(%s) %d(%s) %s",
				sqlite3_extended_errcode(db),
				sqlite3_errstr(sqlite3_extended_errcode(db)),
				res,
				sqlite3_errstr(res),
				sqlite3_errmsg(db));
	abort();
	return res;
}


int db_once(sqlite3_stmt* stmt) {
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

/* before sqlite sucks, have to malloc copies of pointers to all statements,
	 before the database won't close after we finalize them all. */
sqlite3_stmt** stmts = NULL;
size_t nstmt = 0;
void db_add_stmt(sqlite3_stmt* stmt) {
	stmts = realloc(stmts,sizeof(*stmts)*(++nstmt));
	stmts[nstmt-1] = stmt;
}

const char* db_next = NULL;

sqlite3_stmt* db_preparen(const char* s, int l) {
	sqlite3_stmt* stmt = NULL;
	db_check(sqlite3_prepare_v2(db, s, l, &stmt, &db_next)); 
	assert(stmt != NULL);
	return stmt;
}

void db_open() {
	const char* filename = "storyinfo.sqlite";
	if(NULL != getenv("db")) filename = getenv("db");
	db_check(sqlite3_open(filename,&db));
	assert(db != NULL);
	PREPARE(begin_stmt,"BEGIN");
	PREPARE(commit_stmt,"COMMIT");
	PREPARE(rollback_stmt,"ROLLBACK");
	char* err = NULL;
//#define UPGRADEME
#ifndef UPGRADEME
	BEGIN_TRANSACTION;
#endif

	{
#include "schema.sql.gen.c"
		db_check(sqlite3_exec(db, sql, NULL, NULL, &err));
	}
#ifdef UPGRADEME
	{
		#include "upgrade.sql.gen.c"
		db_check(sqlite3_exec(db, sql, NULL, NULL, &err));
	}
	BEGIN_TRANSACTION;
#endif
	
	{
#include "indexes.sql.gen.c"
		db_check(sqlite3_exec(db, sql, NULL, NULL, &err));
	}

#ifdef UPGRADEME
	exit(0);
#endif

	
	db_retransaction();

	{
		sqlite3_stmt* check = NULL;
		db_check(sqlite3_prepare_v2(db, LITLEN(
																	"SELECT 1 FROM committing LIMIT 1"
																	), &check, NULL));
		sqlite3_stmt* ins = NULL;
		db_check(sqlite3_prepare_v2(db, LITLEN(
																	"INSERT INTO committing DEFAULT VALUES"
																	), &ins, NULL));
		for(;;) {
			int res = db_once(check);
			if(res == SQLITE_DONE) {
				// yay, race conditions
				db_once(ins);
			} else {
				assert(res == SQLITE_ROW);
				break;
			}
		}

		sqlite3_finalize(check);
		sqlite3_finalize(ins);
	}

	END_TRANSACTION;

}

void db_close_and_exit(void) {
	if(transaction_level > 0) {
		db_commit();
	}
	size_t i;
	for(i=0;i<nstmt;++i) {
		db_check(sqlite3_finalize(stmts[i]));
	}
	sqlite3_stmt* stmt = NULL;
	while((stmt = sqlite3_next_stmt(db, stmt))) {
		db_check(sqlite3_finalize(stmt));
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

int db_once_trans(sqlite3_stmt* stmt) {
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

void db_saw_commit(git_time_t updated, const db_oid commit) {
	static sqlite3_stmt* update_after = NULL, *update_before;
	if(update_after == NULL) {
		PREPARE(update_after, "UPDATE committing SET after = ?");
		PREPARE(update_before, "UPDATE committing SET before = ? WHERE before IS NOT NULL");
	}
	/* update before every time
		 so if interrupted, we start halfway to the "after" again or somewhere
	 */
	BEGIN_TRANSACTION;
	db_check(sqlite3_bind_blob(update_before, 1, commit, sizeof(db_oid), NULL));
	db_once(update_before);

	if(!saw_commit) {
		/* only update after at the beginning
			 because next time we want to go after the latest commit we saw this time. */
		db_check(sqlite3_bind_int64(update_after, 1, updated));
		db_once(update_after);
		saw_commit = true;
	}
	END_TRANSACTION;
}


void db_caught_up_committing(void) {
	DECLARE_STMT(unset_before,"UPDATE committing SET before = NULL");

	if(saw_commit)
		db_once_trans(unset_before);
}

identifier db_get_category(const string name, git_time_t* updated) {
	DECLARE_STMT(find,"SELECT id,updated FROM categories WHERE category = ?");
	DECLARE_STMT(insert,"INSERT INTO categories (category) VALUES(?)");
	db_check(sqlite3_bind_text(find,1,name.s,name.l,NULL));
	RESETTING(find) int res = sqlite3_step(find);
	TRANSACTION;
	if(res == SQLITE_ROW) {
		*updated = sqlite3_column_int64(find,1);
		return sqlite3_column_int64(find,0);
	}
	*updated = 0;
	db_check(sqlite3_bind_text(insert,1,name.s,name.l,NULL));
	db_once(insert);
	identifier ret = sqlite3_last_insert_rowid(db);
	return ret;
}

void db_last_seen_commit(struct bad* out,
												 git_time_t* after, db_oid before) {
	DECLARE_STMT(find,"SELECT after,before FROM committing LIMIT 1");

	RESETTING(find) int res = sqlite3_step(find);
	assert(res == SQLITE_ROW);
	if(sqlite3_column_type(find,0) != SQLITE_NULL) {
		out->after = true;
		*after = sqlite3_column_int64(find,0);
	}
	if(sqlite3_column_type(find,1) != SQLITE_NULL) {
		out->before = true;
		assert(sizeof(db_oid) == sqlite3_column_bytes(find,1));
		memcpy(before,sqlite3_column_blob(find, 1),sizeof(db_oid));
	}
}

void db_caught_up_category(identifier category) {
	DECLARE_STMT(update,"UPDATE categories SET updated = ? WHERE id = ?");

	time_t updated = time(NULL);
	db_check(sqlite3_bind_int(update,1,updated));
	db_check(sqlite3_bind_int64(update,2,category));
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
//	SPAM("retransaction %d",transaction_level);
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
	db_check(sqlite3_bind_text(find,1,tag,tlen,NULL));
	int res = sqlite3_step(find);
	sqlite3_reset(find);
	db_check(res);
	return res == SQLITE_ROW ? 1 : 0;
}

static struct {
	identifier story;
	identifier chapter;
} working = {};

void db_working_on(identifier story, identifier chapter) {
	working.story = story;
	working.chapter = chapter;
}

void cool_xml_error_handler(void * userData, xmlErrorPtr error) {
	if(htmlish_handled_error(error)) return;
	if(error->code == XML_HTML_UNKNOWN_TAG) {
		const char* name = error->str1;
		size_t nlen = strlen(name);
		if(is_cool_xml_tag(name,nlen)) return;
	}
	if(working.story) {
		DECLARE_STMT(find,"SELECT location,(select title FROM chapters where id = ?2) FROM stories WHERE id = ?1");
		db_check(sqlite3_bind_int64(find,1,working.story));
		db_check(sqlite3_bind_int64(find,2,working.chapter));
		RESETTING(find) int res = db_check(sqlite3_step(find));
		if(res == SQLITE_ROW) {
			fprintf(stderr,"=== %.*s:%ld ",
							sqlite3_column_bytes(find,0),
							(char*)sqlite3_column_blob(find,0),
							working.chapter);
			if(sqlite3_column_type(find,1) != SQLITE_NULL) {
				fprintf(stderr,"(%.*s) ",
								sqlite3_column_bytes(find,1),
								(char*)sqlite3_column_blob(find,1));
			}
		} else {
			fprintf(stderr,"??? %ld:%ld ",working.story,working.chapter);
		}
		fputc('\n',stderr);
		working.story = 0;
		working.chapter = 0;
	}
	fprintf(stderr,"xml error %s %s\n",error->message,
					error->level == XML_ERR_FATAL ? "fatal..." : "ok");
}
