#include "string.h"

#include <stdint.h> // int64_t
#include <git2/oid.h> // GIT_OID_RAWSZ
#include <stdbool.h>

#define DB_OID(o) o.id
typedef unsigned char db_oid[GIT_OID_RAWSZ];

void db_open(const char* filename);
void db_close_and_exit(void);

void db_saw_commit(git_time_t timestamp, db_oid commit);
bool db_last_seen_commit(db_oid commit);

typedef int64_t identifier;

identifier db_find_story(const string location);
void db_saw_chapter(bool deleted, identifier story, git_time_t timestamp, long int chapnum);