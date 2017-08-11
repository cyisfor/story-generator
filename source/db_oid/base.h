#include <git2/oid.h> // GIT_OID_RAWSZ

#define DB_OID(o) o.id
// typedef git_oid.id db_oid
typedef unsigned char db_oid[GIT_OID_RAWSZ];

const git_oid* GIT_OID(const db_oid a);
const char* db_oid_str(const db_oid oid);
