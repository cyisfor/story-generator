#include "storydb.h"

#include "record.h"
#include "itoa.h"
#include "ensure.h"

#include <sqlite3.h>

#include "db-private.h"

#include "db.sql.stmt.c"


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
	string s = { .base = getenv("story") };
	if(s.base != NULL) {
		s.len = strlen(s.base);
		only_story.i = storydb_find_story(s);
		ensure_ge(only_story.i,0);
		only_story.ye = true;
	}
}


identifier storydb_find_story(const string location) {
	DECLARE_STMT(find,"SELECT id FROM stories WHERE location = ?");
	db_check(sqlite3_bind_text(find,1,location.base,location.len,NULL));
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
		db_check(sqlite3_bind_int64(insert,1,story));
		db_once(insert);
	} else {
		db_check(sqlite3_bind_int64(delete,1,story));
		db_once(delete);
	}
	return sqlite3_changes(db) > 0; // operation did something, or not.
}

bool storydb_under_construction(identifier story, identifier chapter) {
	DECLARE_STMT(find,"SELECT under_construction FROM chapters WHERE story = ?1 AND chapter = ?2");
	sqlite3_bind_int64(find,1,story);
	sqlite3_bind_int64(find,2,chapter);
	RESETTING(find) int res = db_check(sqlite3_step(find));
	if(res == SQLITE_ROW) {
		return sqlite3_column_int(find, 0) == 1;
	}
	return true;
}

bool storydb_set_under_construction(identifier story, identifier chapter,
									bool under_construction) {
	DECLARE_STMT(update,"UPDATE chapters SET under_construction = ?3 WHERE story = ?1 AND chapter = ?2 AND under_construction != ?3");
	sqlite3_bind_int64(update,1,story);
	sqlite3_bind_int64(update,2,chapter);
	sqlite3_bind_int(update,3,under_construction ? 1 : 0);
	db_once(update);
	return sqlite3_changes(db) > 0; // operation did something, or not.
}

identifier storydb_get_story(const string location, git_time_t created) {
	DECLARE_STMT(find,"SELECT id FROM stories WHERE location = ?");
//	DECLARE_STMT(finderp,"SELECT id,location FROM stories");
	DECLARE_STMT(insert,"INSERT INTO stories (location,created,updated) VALUES (?1,?2,?2)");

	record(WARNING, "get story %.*s",location.len, location.base);
	db_check(sqlite3_bind_text(find,1,location.base,location.len,NULL));
	TRANSACTION;
	RESETTING(find) int res = db_check(sqlite3_step(find));
	if(res == SQLITE_ROW) {
		identifier id = sqlite3_column_int64(find,0);
		return id;
	} else {
		record(WARNING, "inserderp %s", sqlite3_errstr(res));
		db_check(sqlite3_bind_text(insert,1,location.base, location.len, NULL));
		db_check(sqlite3_bind_int64(insert,2,created));
		db_once(insert);
		identifier story = sqlite3_last_insert_rowid(db);
		record(INFO, "creating story %.*s: %lu",location.len,location.base,story);
		return story;
	}
}

// how low can we stoop?
bool need_restart_for_chapters = false;

void storydb_saw_chapter(bool deleted, identifier story,
										git_time_t updated, identifier chapter) {
	assert(chapter != 0);
	if(deleted) {
		record(INFO, "BALEETED %d:%d %d",story,chapter,updated);
		DECLARE_STMT(delete, "DELETE FROM chapters WHERE story = ? AND chapter = ?");
		db_check(sqlite3_bind_int64(delete,1,story));
		db_check(sqlite3_bind_int64(delete,2,chapter));
		db_once_trans(delete);
		return;
	}
	
	//record(INFO, "SAW %d:%d %d",story,chapter,updated);
	DECLARE_STMT(find,"SELECT updated FROM chapters WHERE story = ? AND chapter = ?");
	DECLARE_STMT(update,
				 "UPDATE chapters SET updated = ?, seen = MAX(seen, updated) "
				 "WHERE story = ? AND chapter = ?");
	DECLARE_STMT(update_max,
				 "UPDATE chapters SET updated = MAX(updated,?), seen = MAX(seen, updated) "
				 "WHERE story = ? AND chapter = ?");
	DECLARE_STMT(update_story,SAW_CHAPTER_UPDATE_STORY);
	DECLARE_STMT(insert,"INSERT INTO chapters (created,updated,seen,story,chapter) VALUES (?1,?1,?1,?,?)");
	DECLARE_STMT(notcreated,"SELECT 1 FROM chapters WHERE created = ?1");
	git_time_t created = updated;
	for(;;) {
		db_check(sqlite3_bind_int64(notcreated, 1, created));
		int res = sqlite3_step(notcreated);
		sqlite3_reset(notcreated);
		if(res == SQLITE_DONE) {
			// XXX: yay, race conditions!
			break;
		} else {
			--created;
		}
	}
	db_check(sqlite3_bind_int64(find,1,story));
	db_check(sqlite3_bind_int64(find,2,chapter));
	assert(updated > 0);
	TRANSACTION;
	RESETTING(find) int res = sqlite3_step(find);
	switch(res) {
	case SQLITE_ROW: {
		// update if new updated is different
		if(sqlite3_column_int64(find,0) != updated) {
			git_time_t derpdated = updated;
			if(!derpdated) derpdated = time(NULL);
			db_check(sqlite3_bind_int64(update,1,updated));
			db_check(sqlite3_bind_int64(update,2,story));
			db_check(sqlite3_bind_int64(update,3,chapter));
			db_once(update);
			assert(sqlite3_changes(db) > 0);
			// any for_chapters iterator has to be restarted now.
			need_restart_for_chapters = true;
		}
		} break;
	case SQLITE_DONE:
		db_check(sqlite3_bind_int64(insert,1,created));
		db_check(sqlite3_bind_int64(insert,2,story));
		db_check(sqlite3_bind_int64(insert,3,chapter));
		db_once(insert);
		record(INFO, "creating chapter %lu:%lu",story,chapter);
		break;
	};

	db_check(sqlite3_bind_int64(update_story, 1, updated));
	db_check(sqlite3_bind_int64(update_story, 2, story));
//	db_check(sqlite3_bind_int64(update_story, 3, storydb_all_ready ? 1 : 0));
	//db_check(sqlite3_bind_int64(update_story, 4, chapter));
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
	db_check(sqlite3_bind_int64(cache->find, 1, story));
	db_check(sqlite3_bind_int64(cache->find, 2, chapter));
	TRANSACTION;
	int res = db_check(sqlite3_step(cache->find));
	sqlite3_reset(cache->find);
	if(res == SQLITE_ROW) return true;
	db_check(sqlite3_bind_int64(cache->insert, 1, story));
	db_check(sqlite3_bind_int64(cache->insert, 2, chapter));
	db_check(sqlite3_step(cache->insert));
	sqlite3_reset(cache->insert);
	return false;
}

void storydb_cache_free(struct storycache* cache) {
	sqlite3_finalize(cache->find);
	sqlite3_finalize(cache->insert);
	free(cache);
}


void storydb_sync_stories_updated(void) {
	DECLARE_STMT(update,"update stories set updated = (select max(updated) from chapters where story = stories.id);");
	db_once(update);
}

/* be CAREFUL none of these iterators are re-entrant! */

void storydb_for_recent_chapters(
	void* udata,
	int limit,
	void (*handle)(
		void* udata,
		identifier story,
		size_t chapnum,
		bool under_construction,
		const string story_title,
		const string chapter_title,
		const string location,
		git_time_t updated)) {

	DECLARE_STMT(find,RECENT_CHAPTERS);
	RESETTING(find) int res;
	db_check(sqlite3_bind_int(find,1,storydb_only_censored ? 1 : 0));
	db_check(sqlite3_bind_int(find,2,storydb_all_ready ? 1 : 0));
	db_check(sqlite3_bind_int(find,3,limit));


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
				.base = sqlite3_column_blob(find,2),
				.len = sqlite3_column_bytes(find,2)
			};
			const string chapter_title = {
				.base = sqlite3_column_blob(find,3),
				.len = sqlite3_column_bytes(find,3)
			};
			const string location = {
				.base = sqlite3_column_blob(find,4),
				.len = sqlite3_column_bytes(find,4)
			};
			bool under_construction = sqlite3_column_int(find,6) == 1;
				
			handle(
				udata,
				sqlite3_column_int64(find,0),
				sqlite3_column_int64(find,1),
				under_construction,
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


void storydb_for_stories(
	void* udata,
	void (*handle)(void* udata,
								 identifier story,
								 const string location,
								 size_t ready,
								 size_t numchaps,
								 git_time_t updated),
	bool forward,
	git_time_t after) {
	if(only_story.ye) {
		DECLARE_STMT(find, FOR_ONLY_STORY);

		db_check(sqlite3_bind_int64(find,1,only_story.i));
		db_check(sqlite3_bind_int(find,2,storydb_only_censored ? 1 : 0));
		RESETTING(find) int res = db_check(sqlite3_step(find));
		ensure_eq(res,SQLITE_ROW);
		const string location = {
			.base = sqlite3_column_blob(find,0),
			.len = sqlite3_column_bytes(find,0)
		};

		handle(udata,
					 only_story.i,
					 location,
					 sqlite3_column_int64(find,1),
					 sqlite3_column_int64(find,2),
					 sqlite3_column_int64(find,3));
		return;
	}

	DECLARE_STMT(findfor,FOR_STORIES);
	// Note: this is why statements2source.c must have onlydefine
	DECLARE_STMT(findrev,FOR_STORIES " DESC");

	sqlite3_stmt* find;
	if(forward)
		find = findfor;
	else
		find = findrev;

	db_check(sqlite3_bind_int(find,1,storydb_all_ready));
	db_check(sqlite3_bind_int64(find,2,after));
	for(;;) {
		int res = sqlite3_step(find);
		switch(res) {
		case SQLITE_ROW: {
			const string location = {
				.base = sqlite3_column_blob(find,1),
				.len = sqlite3_column_bytes(find,1)
			};
			handle(udata,
						 sqlite3_column_int64(find,0),
						 location,
						 sqlite3_column_int64(find,2),
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

void storydb_for_undescribed(
	void* udata,
	void (*handle)(
		void* udata,
		identifier story,
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
			.base = sqlite3_column_blob(find,1),
			.len = sqlite3_column_bytes(find,1)
		};
		string description = {
			.base = sqlite3_column_blob(find,2),
			.len = sqlite3_column_bytes(find,2)
		};
		string source = {
			.base = sqlite3_column_blob(find,3),
			.len = sqlite3_column_bytes(find,3)
		};
		handle(udata, story,title,description,source);
	}
}

void storydb_for_chapters(
	void* udata,
	void (*handle)(
		void* udata,
		identifier chapter,
		git_time_t timestamp),
	identifier story,
	git_time_t after,
	int numchaps,
	bool all_ready) {
	DECLARE_STMT(find,FOR_CHAPTERS);

RESTART:
	db_check(sqlite3_bind_int64(find,1,story));
	db_check(sqlite3_bind_int64(find,2,after));
	db_check(sqlite3_bind_int(find,3,all_ready ? 1 : 0));
	db_check(sqlite3_bind_int(find,4,numchaps));
	for(;;) {
		int res = sqlite3_step(find);
		switch(res) {
		case SQLITE_ROW:
			handle(udata,
						 sqlite3_column_int64(find,0),
						 sqlite3_column_int64(find,1));
			if(need_restart_for_chapters) {
				/* okay, so before in order of ascending updated...
					 we can re-bind a new updated to find,
					 only checking (again) from now onward
					 no chapter should be added with an OLDER updated
					 than the current one!
				*/
				after = sqlite3_column_int64(find,1);
				sqlite3_reset(find);
				db_check(sqlite3_bind_int64(find,2,after));
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

// can't return a string, or the statement will be left un-reset
void storydb_with_chapter_title(
	void* udata,
	void (*handle)(void* udata, const string),
	identifier story,
	identifier chapter) {
	DECLARE_STMT(find,"SELECT title FROM chapters WHERE story = ? AND chapter = ?");
	db_check(sqlite3_bind_int64(find,1,story));
	db_check(sqlite3_bind_int64(find,2,chapter));
	RESETTING(find) int res = sqlite3_step(find);
	if(res == SQLITE_ROW) {
		const string title = {
			.base = sqlite3_column_blob(find,0),
			.len = sqlite3_column_bytes(find,0)
		};
		handle(udata, title);
	} else {
		db_check(res);
		string title = {};
		handle(udata, title);
	}
}

bool derp = false;
void storydb_with_info(
	void* udata,
	void (*handle)(
		void* udata,
		string title,
		string description,
		string source),
	identifier story) {
	assert(derp == false); // not reentrant!
	derp = true;
	DECLARE_STMT(find,"SELECT title,description,source FROM stories WHERE id = ? AND ("
		"title IS NOT NULL OR "
		"description IS NOT NULL OR "
		"source IS NOT NULL)");
	db_check(sqlite3_bind_int64(find,1,story));
	string title = {};
	string description = {};
	string source = {};
	RESETTING(find) int res = sqlite3_step(find);
	if(res == SQLITE_ROW) {
		void CHECK(int col, string* str) {
			str->base = sqlite3_column_blob(find,col);
			str->len = sqlite3_column_bytes(find,col);
		}
		CHECK(0,&title);
		CHECK(1,&description);
		CHECK(2,&source);
	} else {
		db_check(res);
	}
	// handle if nothing in the db, so we can search for files and stuff.
	// XXX: TODO: only call a special creation function if not found?
	handle(udata,title,description,source);
	derp = false;
}

identifier storydb_count_chapters(identifier story) {
	DECLARE_STMT(find,"SELECT COUNT(chapter) FROM chapters WHERE story = ?");
	db_check(sqlite3_bind_int64(find,1,story));
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
	db_check(sqlite3_bind_text(update,1,title.base,title.len,NULL));
	db_check(sqlite3_bind_int64(update,2,story));
	db_check(sqlite3_bind_int64(update,3,chapter));
	db_check(sqlite3_bind_text(update,4,title.base,title.len,NULL));
	BEGIN_TRANSACTION;
	db_once(update);
	if(!*title_changed) {
		if(sqlite3_changes(db) > 0) *title_changed = true;
		// never set title_changed to false, it goes true it stays true
	}
	END_TRANSACTION;
}

void storydb_set_info(identifier story,
											 const string title,
											 const string description,
											 const string source) {
	DECLARE_STMT(update,"UPDATE stories SET title = COALESCE(?,title),"
							 "description = COALESCE(?,description),"
							 "source = COALESCE(?,source) "
							 "WHERE id = ?");
	void one(int col, const string thing) {
		if(thing.len == 0 || thing.base == NULL) {
			db_check(sqlite3_bind_null(update,col));
		} else {
			db_check(sqlite3_bind_text(update,col,thing.base,thing.len,NULL));
		}
	}
	one(1,title);
	one(2,description);
	one(3,source);
	db_check(sqlite3_bind_int64(update,4,story));
	db_once_trans(update);
}

void storydb_set_chapters(identifier story, size_t numchaps) {
	DECLARE_STMT(update,"UPDATE stories SET chapters = ? "
							 "WHERE id = ?");
	db_check(sqlite3_bind_int64(update,1,numchaps));
	db_check(sqlite3_bind_int64(update,2,story));
	db_once_trans(update);
}

	/*
		For unpublished (which > ready) chapters,
		show them in order of oldest mtime to newest
		 - by commit and/or file mtime

		Chapters that have an old mtime, but are unpublished probably
		should be published.
	 */

void storydb_for_unpublished_chapters(
	void* udata,
	int limit,
	void (*handle)(
		void* udata,
		identifier story,
		const string location,
		identifier chapter,
		git_time_t timestamp)) {

	DECLARE_STMT(find,UNPUBLISHED_CHAPTERS);
	db_check(sqlite3_bind_int(find,1,limit));
	for(;;) {
		int res = sqlite3_step(find);
		switch(res) {
		case SQLITE_ROW:
			{
				const string story_location = {
					.base = sqlite3_column_blob(find,1),
					.len = sqlite3_column_bytes(find,1)
				};
				handle(udata,
							 sqlite3_column_int64(find,0),
							 story_location,
							 sqlite3_column_int64(find,2),
							 sqlite3_column_int64(find,3));
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
