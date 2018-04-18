#ifndef _DB_H_
#define _DB_H_

#include "mystring.h"

#include <stdint.h> // int64_t
#include <stdbool.h>

#include "db_oid/base.h"

void db_open();
void db_close_and_exit(void);

void db_set_category(const string category);
void db_saw_commit(git_time_t timestamp, const db_oid commit);
void db_caught_up_committing(void);
struct bad {
	bool after;
	bool before;
};
void db_last_seen_commit(struct bad* out,
												 git_time_t* after, db_oid before);

typedef int64_t identifier;

identifier db_get_category(const string name, git_time_t* timestamp);
void db_caught_up_category(identifier category);

// XXX: just for the error handler...
void db_working_on(identifier story, identifier chapter);

#include <libxml/xmlerror.h>

void cool_xml_error_handler(void * userData, xmlErrorPtr error);

void db_transaction(void (*run)(void));
void db_retransaction(void);

void db_begin(void);
void db_commit(void);
void db_rollback(void);

#define BEGIN_TRANSACTION db_begin()
#define END_TRANSACTION db_commit()

#define CPPSUX(a,b) a ## b
#define CONCAT_SYM(a,b) CPPSUX(a,b)

#define RESETTING(stmt) int CONCAT_SYM(resetter,__LINE__) () {	\
		/* INFO("Resetting " #stmt); */															\
		sqlite3_reset(stmt);																				\
	}																															\
	__attribute__((__cleanup__(CONCAT_SYM(resetter,__LINE__))))

#define TRANSACTION int CONCAT_SYM(committer,__LINE__) () {			\
		/* INFO("Resetting " #stmt); */															\
		db_commit();																								\
	}																															\
	db_begin();																										\
	__attribute__((__cleanup__(CONCAT_SYM(committer,__LINE__))))	\
	int CONCAT_SYM(sentinel,__LINE__)

#endif /* _DB_H_ */
