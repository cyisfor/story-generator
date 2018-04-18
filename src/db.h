#ifndef _DB_H_
#define _DB_H_

#include "mystring.h"

#include <stdint.h> // int64_t
#include <stdbool.h>

// censor stories that are "bad"
extern bool db_only_censored;
// include last chapter for all stories
extern bool db_all_finished;




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

bool db_set_censored(identifier story, bool censored);

identifier db_find_story(const string location);
identifier db_get_story(const string location, git_time_t timestamp);


void db_saw_chapter(bool deleted, identifier story,
										git_time_t timestamp, identifier chapter);


struct storycache;

struct storycache* db_start_storycache(void);
// return true if it's in the cache, add it if not in the cache.
bool db_in_storycache(struct storycache* cache, identifier story, size_t chapter);
void db_storycache_free(struct storycache* cache);


/* be CAREFUL none of these iterators are re-entrant! */

void db_for_recent_chapters(int limit,
														void (*handle)(identifier story,
																					 size_t chapnum,
																					 const string story_title,
																					 const string chapter_title,
																					 const string location,
																					 git_time_t timestamp));

void db_for_stories(void (*handle)(identifier story,
																	 const string location,
																	 bool finished,																	 
																	 size_t numchaps,
																	 git_time_t timestamp),
										bool forward,
										git_time_t after);

void db_for_undescribed_stories(void (*handle)(identifier story,
																							 const string title,
																							 const string description,
																							 const string source));

void db_for_chapters(identifier story,
										 void (*handle)(identifier chapter,
																		git_time_t timestamp),
										 git_time_t after,
										 bool only_ready);

void db_with_chapter_title(identifier story,
													 identifier chapter,
													 void (*handle)(const string));

void db_with_story_info(const identifier story, void (*handle)(const string title,
																															 const string description,
																															 const string source));

// for db_set_* empty strings will set the db value to NULL
void db_set_story_info(identifier story,
											 const string title,
											 const string description,
											 const string source)
;
void db_set_chapters(identifier story, size_t numchaps);

void db_get_chapter_title(string* dest, identifier story, identifier chapter);
void db_get_story_title(string* dest, identifier story);
void db_get_story_description(string* dest, identifier story);
void db_get_story_source(string* dest, identifier story);

identifier db_count_chapters(identifier story);

// db should set to NULL if string is empty
void db_set_chapter_title(const string title,
													identifier story, identifier chapter,
													bool* title_changed);

void db_set_story_info(identifier story,
											 const string title,
											 const string description,
											 const string source);

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
