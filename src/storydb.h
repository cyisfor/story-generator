#include "db.h"
// censor stories that are "bad"
extern bool storydb_only_censored;
// include last chapter for all stories
extern bool storydb_all_ready;


bool storydb_set_censored(identifier story, bool censored);

identifier storydb_find_story(const string location);
identifier storydb_get_story(const string location, git_time_t timestamp);


void storydb_saw_chapter(bool deleted, identifier story,
										git_time_t timestamp, identifier chapter);

struct storycache;

struct storycache* storydb_start_cache(void);
// return true if it's in the cache, add it if not in the cache.
bool storydb_in_cache(struct storycache* cache, identifier story, size_t chapter);
void storydb_cache_free(struct storycache* cache);


/* be CAREFUL none of these iterators are re-entrant! */

void storydb_for_recent_chapters(int limit,
														void (*handle)(identifier story,
																					 size_t chapnum,
																					 const string story_title,
																					 const string chapter_title,
																					 const string location,
																					 git_time_t timestamp));

void storydb_for_stories(void (*handle)(identifier story,
																				const string location,
																				size_t ready,
																				size_t numchaps,
																				git_time_t timestamp),
										bool forward,
										git_time_t after);

void storydb_for_undescribed_stories(void (*handle)(identifier story,
																							 const string title,
																							 const string description,
																							 const string source));

void storydb_for_chapters(identifier story,
										 void (*handle)(identifier chapter,
																		git_time_t timestamp),
										 git_time_t after,
										 bool only_ready);

void storydb_with_chapter_title(identifier story,
													 identifier chapter,
													 void (*handle)(const string));

void storydb_with_story_info(const identifier story, void (*handle)(const string title,
																															 const string description,
																															 const string source));

// for storydb_set_* empty strings will set the db value to NULL
void storydb_set_story_info(identifier story,
											 const string title,
											 const string description,
											 const string source)
;
void storydb_set_chapters(identifier story, size_t numchaps);

void storydb_get_chapter_title(string* dest, identifier story, identifier chapter);
void storydb_get_story_title(string* dest, identifier story);
void storydb_get_story_description(string* dest, identifier story);
void storydb_get_story_source(string* dest, identifier story);

identifier storydb_count_chapters(identifier story);

// db should set to NULL if string is empty
void storydb_set_chapter_title(const string title,
													identifier story, identifier chapter,
													bool* title_changed);

void storydb_set_info(identifier story,
											const string title,
											const string description,
											const string source);