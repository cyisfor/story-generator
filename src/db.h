#ifndef _DB_H_
#define _DB_H_

#include "string.h"

#include <stdint.h> // int64_t
#include <stdbool.h>

#include "db_oid/base.h"

void db_open(const char* filename);
void db_close_and_exit(void);

void db_saw_commit(git_time_t timestamp, db_oid commit);
void db_caught_up(void);
struct bad {
	bool last;
	bool current;
};
void db_last_seen_commit(struct bad* out,
												 db_oid last, db_oid current,
												 git_time_t* timestamp);

typedef int64_t identifier;

identifier db_find_story(const string location, git_time_t timestamp);

void db_saw_chapter(bool deleted, identifier story,
										git_time_t timestamp, identifier chapter);


void db_for_stories(void (*handle)(identifier story,
																	 const string location,
																	 bool finished,																	 
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

void db_with_story_info(const identifier story, void (*handle)(const string title,
																															 const string description,
																															 const string source));

// for db_set_* empty strings will set the db value to NULL
void db_set_story_info(identifier story,
											 const string title,
											 const string description,
											 const string source);

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

#include <libxml/xmlerror.h>

void cool_xml_error_handler(void * userData, xmlErrorPtr error);

void db_transaction(void (*run)(void));
void db_retransaction(void);
#define BEGIN_TRANSACTION(name) void intrans ## name(void) {
#define END_TRANSACTION(name) }; db_transaction(intrans ## name);

#endif /* _DB_H_ */