#include "storydb.h"
#include <sqlite3.h>

#include "db-private.h"

#include "o/db.sql.c"

#include "itoa.h"
#include "ensure.h"


#include <assert.h>

bool storydb_only_censored = false;
bool storydb_all_ready = false;

static
struct {
	bool ye;
	identifier i;
} only_story = {};



void storydb_open() {
	db_open();
	string s = { .s = getenv("story") };
	if(s.s != NULL) {
		s.l = strlen(s.s);
		only_story.i = storydb_find_story(s);
		ensure_ge(only_story.i,0);
		only_story.ye = true;
	}
}


identifier storydb_find_story(const string location) {
	DECLARE_STMT(find,"SELECT id FROM stories WHERE location = ?");
	sqlite3_bind_text(find,1,location.s,location.l,NULL);
	RESETTING(find) int res = db_check(sqlite3_step(find));
	if(res == SQLITE_ROW) {
		return sqlite3_column_int64(find,0);
	}
	return -1;
}

bool storydb_set_censored(identifier story, bool censored) {
	DECLARE_STMT(insert,"INSERT OR IGNORE INTO censored_stories (story) VALUES (?)");
	DECLARE_STMT(delete,"DELETE FROM censored_stories WHERE story = ?");
	if(censored) {
		sqlite3_bind_int64(insert,1,story);
		db_once(insert);
	} else {
		sqlite3_bind_int64(delete,1,story);
		db_once(delete);
	}
	return sqlite3_changes(db) > 0; // operation did something, or not.
}

identifier storydb_get_story(const string location, git_time_t created) {
	DECLARE_STMT(find,"SELECT id FROM stories WHERE location = ?");
	DECLARE_STMT(insert,"INSERT INTO stories (location,created,updated) VALUES (?1,?2,?2)");

	sqlite3_bind_text(find,1,location.s,location.l,NULL);
	TRANSACTION;
	RESETTING(find) int res = db_check(sqlite3_step(find));
	if(res == SQLITE_ROW) {
		identifier id = sqlite3_column_int64(find,0);
		return id;
	} else {
		sqlite3_bind_text(insert,1,location.s, location.l, NULL);
		sqlite3_bind_int64(insert,2,created);
		db_once(insert);
		identifier story = sqlite3_last_insert_rowid(db);
		INFO("creating story %.*s: %lu",location.l,location.s,story);
		return story;
	}
}

// how low can we stoop?
bool need_restart_for_chapters = false;

void storydb_saw_chapter(bool deleted, identifier story,
										git_time_t updated, identifier chapter) {
	if(deleted) {
		INFO("BALEETED %d:%d %d",story,chapter,updated);
		DECLARE_STMT(delete, "DELETE FROM chapters WHERE story = ? AND chapter = ?");
		sqlite3_bind_int64(delete,1,story);
		sqlite3_bind_int64(delete,2,chapter);
		db_once_trans(delete);
		return;
	}
	
	//INFO("SAW %d:%d %d",story,chapter,updated);
	DECLARE_STMT(find,"SELECT updated FROM chapters WHERE story = ? AND chapter = ?");
	DECLARE_STMT(update,"UPDATE chapters SET updated = MAX(updated,?), seen = MAX(seen, updated) "
							 "WHERE story = ? AND chapter = ?");
	DECLARE_STMT(update_story,SAW_CHAPTER_UPDATE_STORY);
	DECLARE_STMT(insert,"INSERT INTO chapters (created,updated,seen,story,chapter) VALUES (?1,?1,?1,?,?)");
	sqlite3_bind_int64(find,1,story);
	sqlite3_bind_int64(find,2,chapter);
	TRANSACTION;
	RESETTING(find) int res = sqlite3_step(find);
	switch(res) {
	case SQLITE_ROW:
		// update if new updated is higher
		if(sqlite3_column_int64(find,0) < updated) {
			sqlite3_bind_int64(update,1,updated);
			sqlite3_bind_int64(update,2,story);
			sqlite3_bind_int64(update,3,chapter);
			db_once(update);
			assert(sqlite3_changes(db) > 0);
			// any for_chapters iterator has to be restarted now.
			need_restart_for_chapters = true;
		}
		return;
	case SQLITE_DONE:
		sqlite3_bind_int64(insert,1,updated);
		sqlite3_bind_int64(insert,2,story);
		sqlite3_bind_int64(insert,3,chapter);
		db_once(insert);
		INFO("creating chapter %lu:%lu",story,chapter);
		return;
	};

	sqlite3_bind_int64(update_story, 1, updated);
	sqlite3_bind_int64(update_story, 2, story);
	sqlite3_bind_int64(update_story, 3, storydb_all_ready ? 1 : 0);
	sqlite3_bind_int64(update_story, 4, chapter);
	db_once(update_story);
}

struct storycache {
	sqlite3_stmt* find;
	sqlite3_stmt* insert;
};

struct storycache* storydb_start_cache(void) {
	static int counter = 0;
#define PREFIX "CREATE TEMPORARY TABLE storycache"
	{
		sqlite3_stmt* create;
		char sql[] = PREFIX "XXXX (\n"
			"story INTEGER,\n"
			"chapter INTEGER,\n"
			"PRIMARY KEY(story,chapter)) WITHOUT ROWID";
		ensure_eq(true,itoa(sql+sizeof(PREFIX)-1,4,++counter));
		// the XXX stay, so it's still sizeof(sql)
		db_check(sqlite3_prepare_v2(db, sql, sizeof(sql), &create, NULL));
		assert(create != NULL);

		db_check(sqlite3_step(create));
		sqlite3_finalize(create);
	}
#undef PREFIX

	struct storycache* cache = calloc(1, sizeof(struct storycache));


#define PREFIX "SELECT 1 FROM storycache"
	{
		sqlite3_stmt* create;
		char sql[] = PREFIX "XXXX WHERE story = ? AND chapter = ?";
		ensure_eq(true,itoa(sql+sizeof(PREFIX)-1, 4, counter));
		// the XXX stay, so it's still sizeof(sql)
		db_check(sqlite3_prepare_v2(db, sql, sizeof(sql)-1, &cache->find, NULL));
		assert(cache->find != NULL);
		//add_stmt(cache->find); ehhh....
	}
#undef PREFIX

#define PREFIX "INSERT INTO storycache"
	{
		char sql[] = PREFIX "XXXX (story,chapter) VALUES (?,?)";
		ensure_eq(true,itoa(sql+sizeof(PREFIX)-1, 4, counter));
		// the XXX stay, so it's still sizeof(sql)
		db_check(sqlite3_prepare_v2(db, sql, sizeof(sql), &cache->insert, NULL));
		assert(cache->insert != NULL);
		//add_stmt(cache->insert); ehhh....
	}
#undef PREFIX
	return cache;
}

// return true if it's in the cache, add it if not in the cache->
bool storydb_in_cache(struct storycache* cache, identifier story, size_t chapter) {
	sqlite3_bind_int64(cache->find, 1, story);
	sqlite3_bind_int64(cache->find, 2, chapter);
	TRANSACTION;
	int res = db_check(sqlite3_step(cache->find));
	sqlite3_reset(cache->find);
	if(res == SQLITE_ROW) return true;
	sqlite3_bind_int64(cache->insert, 1, story);
	sqlite3_bind_int64(cache->insert, 2, chapter);
	db_check(sqlite3_step(cache->insert));
	sqlite3_reset(cache->insert);
	return false;
}

void storydb_cache_free(struct storycache* cache) {
	sqlite3_finalize(cache->find);
	sqlite3_finalize(cache->insert);
	free(cache);
}


/* be CAREFUL none of these iterators are re-entrant! */

void storydb_for_recent_chapters(int limit,
														void (*handle)(identifier story,
																					 size_t chapnum,
																					 const string story_title,
																					 const string chapter_title,
																					 const string location,
																					 git_time_t updated)) {

	DECLARE_STMT(find,RECENT_CHAPTERS);
	RESETTING(find) int res;
	sqlite3_bind_int(find,1,storydb_only_censored ? 1 : 0);
	sqlite3_bind_int(find,2,storydb_all_ready ? 1 : 0);
	sqlite3_bind_int(find,3,limit);


	for(;;) {
		res = db_check(sqlite3_step(find));
		switch(res) {
		case SQLITE_ROW: {
#ifdef DEBUGGINGTHISQUERY
			SPAM("erg %d:%d nchap %d fins %d censor %d\n",
					 sqlite3_column_int64(find,0),
					 sqlite3_column_int64(find,1),
					 sqlite3_column_int(find,5),
						sqlite3_column_int(find,6),
						sqlite3_column_int(find,7));
#endif
			const string story_title = {
				.s = sqlite3_column_blob(find,2),
				.l = sqlite3_column_bytes(find,2)
			};
			const string chapter_title = {
				.s = sqlite3_column_blob(find,3),
				.l = sqlite3_column_bytes(find,3)
			};
			const string location = {
				.s = sqlite3_column_blob(find,4),
				.l = sqlite3_column_bytes(find,4)
			};
			handle(
				sqlite3_column_int64(find,0),
				sqlite3_column_int64(find,1),
				story_title,
				chapter_title,
				location,
				sqlite3_column_int64(find,5));
			continue;
		}
		case SQLITE_DONE:
			return;
		};
	}
}

void storydb_for_stories(void (*handle)(identifier story,
																	 const string location,
																	 size_t ready,
																	 size_t numchaps,
																	 git_time_t updated),
										bool forward,
										git_time_t before) {
	if(only_story.ye) {
		DECLARE_STMT(find, FOR_ONLY_STORY);

		sqlite3_bind_int64(find,1,only_story.i);
		sqlite3_bind_int(find,2,storydb_only_censored ? 1 : 0);
		RESETTING(find) int res = db_check(sqlite3_step(find));
		ensure_eq(res,SQLITE_ROW);
		const string location = {
			.s = sqlite3_column_blob(find,0),
			.l = sqlite3_column_bytes(find,0)
		};
		handle(only_story.i,
					 location,
					 sqlite3_column_int64(find,1),
					 sqlite3_column_int64(find,2),
					 sqlite3_column_int64(find,3));
		return;
	}

	DECLARE_STMT(findfor,FOR_STORIES);
	DECLARE_STMT(findrev,FOR_STORIES " DESC");\

	sqlite3_stmt* find;
	if(forward)
		find = findfor;
	else
		find = findrev;

	sqlite3_bind_int64(find,1,before);
	for(;;) {
		int res = sqlite3_step(find);
		switch(res) {
		case SQLITE_ROW: {
			const string location = {
				.s = sqlite3_column_blob(find,1),
				.l = sqlite3_column_bytes(find,1)
			};
			handle(sqlite3_column_int64(find,0),
						 location,
						 sqlite3_column_int(find,2) == 1,
						 sqlite3_column_int64(find,3),
						 sqlite3_column_int64(find,4));
			continue;
		}
		case SQLITE_DONE:
			sqlite3_reset(find);
			return;
		default:
			db_check(res);
			abort();
		};
	}
}

void storydb_for_undescribed_stories(void (*handle)(identifier story,
																							 const string title,
																							 const string description,
																							 const string source)) {
	DECLARE_STMT(find,FOR_UNDESCRIBED_STORIES);
	for(;;) {
		RESETTING(find) int res = db_check(sqlite3_step(find));
		if(res == SQLITE_DONE) return;
		ensure_eq(res,SQLITE_ROW);
		identifier story = sqlite3_column_int64(find,0);
		string title = {
			.s = sqlite3_column_blob(find,1),
			.l = sqlite3_column_bytes(find,1)
		};
		string description = {
			.s = sqlite3_column_blob(find,2),
			.l = sqlite3_column_bytes(find,2)
		};
		string source = {
			.s = sqlite3_column_blob(find,3),
			.l = sqlite3_column_bytes(find,3)
		};
		handle(story,title,description,source);
	}
}


void storydb_for_chapters(identifier story,
										void (*handle)(identifier chapter,
																	 git_time_t updated),
										git_time_t before,
										bool only_ready) {
	DECLARE_STMT(find,FOR_CHAPTERS);

#undef READY
RESTART:
	sqlite3_bind_int64(find,1,story);
	sqlite3_bind_int64(find,2,before);
	sqlite3_bind_int(find,3,only_ready ? 0 : 1);
	for(;;) {
		int res = sqlite3_step(find);
		switch(res) {
		case SQLITE_ROW:
			handle(sqlite3_column_int64(find,0),
						 sqlite3_column_int64(find,1));
			if(need_restart_for_chapters) {
				/* okay, so before in order of ascending updated...
					 we can re-bind a new updated to find,
					 only checking (again) from now onward
					 no chapter should be added with an OLDER updated
					 than the current one!
				*/
				before = sqlite3_column_int64(find,1);
				sqlite3_bind_int64(find,2,before);
				sqlite3_reset(find);
				need_restart_for_chapters = false;
			}
			continue;
		case SQLITE_DONE:
			sqlite3_reset(find);
			return;
		default:
			db_check(res);
			abort();
		};
	}
}

void storydb_with_chapter_title(identifier story,
													 identifier chapter,
													 void (*handle)(const string)) {
	DECLARE_STMT(find,"SELECT title FROM chapters WHERE story = ? AND chapter = ?");
	sqlite3_bind_int64(find,1,story);
	sqlite3_bind_int64(find,2,chapter);
	RESETTING(find) int res = sqlite3_step(find);
	if(res == SQLITE_ROW) {
		const string title = {
			.s = sqlite3_column_blob(find,0),
			.l = sqlite3_column_bytes(find,0)
		};
		handle(title);
	} else {
		db_check(res);
		string title = {};
		handle(title);
	}
}

bool derp = false;
void storydb_with_story_info(const identifier story, void (*handle)(string title,
																												 string description,
																												 string source)) {
	assert(derp == false); // not reentrant!
	derp = true;
	DECLARE_STMT(find,"SELECT title,description,source FROM stories WHERE id = ? AND ("
		"title IS NOT NULL OR "
		"description IS NOT NULL OR "
		"source IS NOT NULL)");
	sqlite3_bind_int64(find,1,story);
	string title = {};
	string description = {};
	string source = {};
	RESETTING(find) int res = sqlite3_step(find);
	if(res == SQLITE_ROW) {
		void CHECK(int col, string* str) {
			str->s = sqlite3_column_blob(find,col);
			str->l = sqlite3_column_bytes(find,col);
		}
		CHECK(0,&title);
		CHECK(1,&description);
		CHECK(2,&source);
	} else {
		db_check(res);
	}
	// handle if nothing in the db, so we can search for files and stuff.
	// XXX: TODO: only call a special creation function if not found?
	handle(title,description,source);
	derp = false;
}

identifier storydb_count_chapters(identifier story) {
	DECLARE_STMT(find,"SELECT COUNT(chapter) FROM chapters WHERE story = ?");
	sqlite3_bind_int64(find,1,story);
	RESETTING(find) int res = sqlite3_step(find);
	if(res == SQLITE_ROW) {
		identifier count = sqlite3_column_int64(find,0);
		return count;
	}
	db_check(res);
	// COUNT(...) always returns rows!
	abort();
}

// should set to NULL if string is empty
void storydb_set_chapter_title(const string title,
													identifier story, identifier chapter,
													bool* title_changed) {
	DECLARE_STMT(update,"UPDATE chapters SET title = ? WHERE story = ? AND chapter = ? AND (title IS NULL OR title != ?)");
	sqlite3_bind_text(update,1,title.s,title.l,NULL);
	sqlite3_bind_int64(update,2,story);
	sqlite3_bind_int64(update,3,chapter);
	sqlite3_bind_text(update,4,title.s,title.l,NULL);
	BEGIN_TRANSACTION;
	db_once(update);
	if(!*title_changed) {
		if(sqlite3_changes(db) > 0) *title_changed = true;
		// never set title_changed to false, it goes true it stays true
	}
	END_TRANSACTION;
}

void storydb_set_story_info(identifier story,
											 const string title,
											 const string description,
											 const string source) {
	DECLARE_STMT(update,"UPDATE stories SET title = COALESCE(?,title),"
							 "description = COALESCE(?,description),"
							 "source = COALESCE(?,source) "
							 "WHERE id = ?");
	void one(int col, const string thing) {
		if(thing.l == 0 || thing.s == NULL) {
			sqlite3_bind_null(update,col);
		} else {
			sqlite3_bind_text(update,col,thing.s,thing.l,NULL);
		}
	}
	one(1,title);
	one(2,description);
	one(3,source);
	sqlite3_bind_int64(update,4,story);
	db_once_trans(update);
}

void storydb_set_chapters(identifier story, size_t numchaps) {
	DECLARE_STMT(update,"UPDATE stories SET chapters = ? "
							 "WHERE id = ?");
	sqlite3_bind_int64(update,1,numchaps);
	sqlite3_bind_int64(update,2,story);
	db_once_trans(update);
}


	
