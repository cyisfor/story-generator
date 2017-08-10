#include "string.h"

#include <stdint.h> // int64_t
#include <git2/oid.h> // GIT_OID_RAWSZ
#include <stdbool.h>

#define DB_OID(o) o.id
typedef unsigned char db_oid[GIT_OID_RAWSZ];

void db_open(const char* filename);
void db_close_and_exit(void);

void db_saw_commit(git_time_t timestamp, db_oid commit);
bool db_last_seen_commit(db_oid commit, git_time_t* timestamp);

typedef int64_t identifier;

identifier db_find_story(const string location);
identifier db_find_story_derp(const string location, git_time_t timestamp);

void db_saw_chapter(bool deleted, identifier story,
										git_time_t timestamp, identifier chapter);


void db_for_stories(void (*handle)(identifier story,
																 const string location,
																 size_t numchaps,
																 git_time_t timestamp),
									git_time_t since);

void db_for_chapters(identifier story,
										void (*handle)(identifier chapter,
																	 git_time_t timestamp),
										git_time_t since);

void db_with_chapter_title(identifier story,
													 identifier chapter,
													 void (*handle)(const string));

void db_with_story_info(identifier story, void (*handle)(const string title,
																												 const string description,
																												 const string source));

// for db_set_* empty strings will set the db value to NULL
void db_set_story_info(identifier story,
											 const string title,
											 const string description,
											 const string source);

void db_set_chapter_title(const string title, identifier story, identifier chapter);

void db_get_chapter_title(string* dest, identifier story, identifier chapter);
void db_get_story_title(string* dest, identifier story);
void db_get_story_description(string* dest, identifier story);
void db_get_story_source(string* dest, identifier story);

// should set to NULL if string is empty
void db_set_chapter_title(const string title, identifier story, identifier chapter);
void db_set_story_info(identifier story,
											 const string title,
											 const string description,
											 const string source);

void db_transaction(void (*run)(void));
void db_retransaction(void);
