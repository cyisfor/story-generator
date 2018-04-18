// meh

extern sqlite3* db;

sqlite3_stmt* db_preparen(const char* s, int l);
extern const char* db_next;

int db_once_trans(sqlite3_stmt* stmt);
int db_once(sqlite3_stmt* stmt);
int db_check(int res);

void db_add_stmt(sqlite3_stmt* stmt);

#define PREPARE(stmt,sql) {																	 \
		stmt = db_preparen(LITLEN(sql));													 \
		db_add_stmt(stmt);																						 \
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

// call storydb_open please
void db_open();
