// meh

extern sqlite3* db;

sqlite3_stmt* db_preparen(const char* s, int l);
extern const char* db_next;

int db_once_trans(sqlite3_stmt* stmt);
int db_once(sqlite3_stmt* stmt);
