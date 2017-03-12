import sqlite3;
import std.exception: enforce;
import std.conv: to;

struct Statement {
	Database* db;
	sqlite3_stmt* stmt;

	void finalize() {
		enforce(SQLITE_OK == sqlite3_finalize(stmt),
						db.errmsg());
	}
	
	void reset() {
		enforce(SQLITE_OK == sqlite3_reset(stmt),
						db.errmsg());
	}

	void go(Args...)(Args args) {
		static if(Args.length > 0) {
			bind(args);
		}
		try {
			enforce(SQLITE_DONE == sqlite3_step(stmt),
							db.errmsg());
		} finally {
			sqlite3_reset(stmt);
		}
	}

	alias opCall = go;

	bool next() {
		switch(sqlite3_step(stmt)) {
		case SQLITE_DONE:
			return false;
		case SQLITE_ROW:
			return true;
		default:
			db.error();
		}
		return false;
	}

	bool opBool() {
		return next();
	}

	T resetting(T)(T delegate(ref Statement) handler) {
		try {
			return handler(this);
		} finally {
			sqlite3_reset(stmt);
		}
	}

	void bind(T)(int col, T value) {
		static if(is(T == int)) {
			enforce(SQLITE_OK == sqlite3_bind_int(stmt,col,value),
													sqlite3_errmsg(*db));
		} else static if(is(T == sqlite3_int64)) {
			enforce(SQLITE_OK == sqlite3_bind_int64(stmt,col,value),
							db.errmsg());
		} else static if(is(T == double) || is(T == float)) {
			enforce(SQLITE_OK == sqlite3_bind_double(stmt,col,value),
							db.errmsg());
		} else static if(is(T == string) || is(T == char[]) || is(T == byte[])) {
			enforce(SQLITE_OK == sqlite3_bind_blob(stmt,col,
																						 value.ptr,cast(int)value.length,null),
							db.errmsg());
		} else {
			string blob = value.to!string;
			enforce(SQLITE_OK == sqlite3_bind_blob(stmt, col,
																						 blob.ptr, cast(int)blob.length,
																						 null),
							db.errmsg());
		}
	}

	void bind(T, Args...)(T t, Args args) {
		bind(1, t);
		foreach (index, _; Args)
			bind(index + 2, args[index]);
	}
	
	T at(T)(int col) {
		static if(is(T == int)) {
			return sqlite3_column_int(stmt,col);
		} else static if(is(T == bool)) {
			return sqlite3_column_int(stmt,col) != 0;
		} else static if(is(T == sqlite3_int64)) {
			return sqlite3_column_int64(stmt,col);
		} else static if(is(T == double) || is(T == float)) {
			return sqlite3_column_double(stmt,col);
		} else static if(is(T == char[]) || is(T == byte[]) || is(T == ubyte[])) {
			const(ubyte)[] p = (sqlite3_column_blob(stmt,col))[0..sqlite3_column_bytes(stmt,col)];
			return p.to!T;
		} else static if(is(T == string)) {
			import print: print;
			print("BEYTH",col,sqlite3_column_bytes(stmt,col));
			import std.string: fromStringz;
			print("uhhh",(cast(const(char)*)sqlite3_column_text(stmt,col)).fromStringz);
			const(ubyte)[] p = (sqlite3_column_blob(stmt,col))[0..sqlite3_column_bytes(stmt,col)];
			return cast(string)p;
		} else {
			static assert(false,"What type is this?");
		}
	}
	
	T as(T)() if (is(T == struct)) {
		import std.meta : staticMap;
		import std.traits : NameOf, isNested, FieldTypeTuple, FieldNameTuple;

		alias FieldTypes = FieldTypeTuple!T;
		enforce(FieldTypes.length == sqlite3_column_count(stmt),"wrong number of columns in result");
		T obj;
		foreach (i, fieldName; FieldNameTuple!T)
			__traits(getMember, obj, fieldName) = at!(FieldTypes[i])(i);
		return obj;
	}

};

class DBException: Exception {
	this(string msg, string file = __FILE__, size_t line = __LINE__) {
		super(msg, file, line);
	}
}

struct Database {
  sqlite3* db;

  Statement begin;
  Statement commit;
  Statement rollback;

  void close() {
		begin.finalize();
		commit.finalize();
		rollback.finalize();
		sqlite3_stmt* stmt = null;
		for(;;) {
			stmt = sqlite3_next_stmt(db,stmt);
			if(stmt == null) break;
			assert(SQLITE_OK == sqlite3_finalize(stmt));
		}
		sqlite3_close(db);
  }
  this(string path) {
		import std.string: toStringz;
		enforce(SQLITE_OK == sqlite3_open(path.toStringz, &db),"couldn't open " ~ path);

		begin = this.prepare("BEGIN");
		commit = this.prepare("COMMIT");
		rollback = this.prepare("ROLLBACK");
  }

	void error() {
		throw new DBException(sqlite3_errmsg(db).to!string);
	}
	string errmsg() {
		import std.string: fromStringz;
		return sqlite3_errmsg(db).fromStringz.to!string;
	}

	void run(string stmts) {
		assert(stmts.length < int.max);
		int left = cast(int)stmts.length;
		const(char)* sql = stmts.ptr;
		while(left > 0) {
			const(char*) tail = null;
			sqlite3_stmt* p = null;

			sqlite3_prepare_v2(db,
												 sql,
												 left,
												 &p,
												 &tail);

			sqlite3_step(p);
			sqlite3_finalize(p);
			
			if(tail == null) break;
			left -= (tail-sql);
			sql = tail;
		}
	}

	Statement prepare(string sql) {
		Statement result = {db:&this,
												stmt:null};
		assert(sql.length < int.max);
		enforce(SQLITE_OK == sqlite3_prepare_v2(db,
																						sql.ptr,
																						cast(int)sql.length,
																						&result.stmt,
																						null),
						errmsg());
		return result;
	}

	int transaction_depth = 0;

	static struct Transaction {
		Database* db;
		bool committed = false;
		~this() {
			if(committed) return;
			--db.transaction_depth;
			if(db.transaction_depth == 0) {
				db.rollback();
			}
		}
		void commit() {
			--db.transaction_depth;
			if(db.transaction_depth == 0) {
				db.commit();
			}
			committed = true;
		}
	};

	Transaction transaction() {
		Transaction t = {
			db: &this,
			committed: false
		};
		if(transaction_depth == 0) {
			begin();
		}
		++transaction_depth;
		return t;
	}
}
