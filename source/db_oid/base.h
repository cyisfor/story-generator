#include <git2/oid.h> // GIT_OID_RAWSZ

#define DB_OID(o) o.id
// typedef git_oid.id db_oid
typedef unsigned char db_oid[GIT_OID_RAWSZ];

git_oid* GIT_OID(db_oid a);
