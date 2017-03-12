import d2sqlite3.sqlite3;
import std.exception: enforce;
import std.conv: to;

extern (C) void function(void*) nullfunc = null;

struct Statement {
	Database* db;
	sqlite3_stmt* stmt;

	void finalize() {
		enforce(SQLITE_OK == sqlite3_finalize(stmt));
	}
	
	void reset() {
		enforce(SQLITE_OK == sqlite3_reset(stmt));
	}

	void go(Args...)(Args args) {
		bind(args);
		try {
			enforce(SQLITE_DONE == sqlite3_step(stmt));
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
			enforce(SQLITE_OK == sqlite3_bind_int(stmt,col,value));
		} else static if(is(T == sqlite3_int64)) {
			enforce(SQLITE_OK == sqlite3_bind_int64(stmt,col,value));
		} else static if(is(T == double) || is(T == float)) {
			enforce(SQLITE_OK == sqlite3_bind_double(stmt,col,value));
		} else static if(is(T == string) || is(T == char[]) || is(T == byte[])) {
			enforce(SQLITE_OK == sqlite3_bind_blob(stmt,col,
																						 value.ptr,cast(int)value.length,null));
		} else {
			string blob = value.to!string;
			enforce(SQLITE_OK == sqlite3_bind_blob(stmt, col,
																						 blob.ptr, cast(int)blob.length,
																						 nullfunc));
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
		} else static if(is(T == sqlite3_int64)) {
			return sqlite3_column_int64(stmt,col);
		} else static if(is(T == double) || is(T == float)) {
			return sqlite3_column_double(stmt,col);
		} else static if(is(T == string) || is(T == char[]) || is(T == byte[])) {
			byte[] result;
			result.length = sqlite3_column_bytes(stmt,col);
			memcpy(result.ptr, sqlite3_column_blob(stmt,col), result.length);
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

class Database {
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
  void init(string path) {
		import std.string: toStringz;
		enforce(SQLITE_OK == sqlite3_open(path.toStringz, &db));

		begin = this.prepare("BEGIN");
		commit = this.prepare("COMMIT");
		rollback = this.prepare("ROLLBACK");
  }

	void error() {
		throw new DBException(sqlite3_errmsg(db).to!string);
	}

	void run(string stmts) {
		size_t left = stmts.length;
		const(char*) sql = stmts.ptr;
		while(left > 0) {
			const(char*) tail = null;
			Statement p;

			sqlite3_prepare_v2(&db,
												 sql,
												 left,
												 &p.stmt,
												 &tail);

			p.step();
			p.finalize();
			
			if(tail == null) break;
			left -= (tail-sql);
			sql = tail;
		}
	}

	Statement prepare(string sql) {
		Statement result = Statement(this,null);
		enforce(SQLITE_OK == sqlite3_prepare_v2(&db,
																						sql,
																						sql.length,
																						&result.stmt,
																						null));
		return result;
	}
}

struct Transaction {
  bool committed;
  ~this() {
		if(!committed)
			db.rollback();
  }
  void commit() {
		db.commit();
		committed = true;
  }
};

Transaction transaction() {
  Transaction t;
  t.committed = false;
  db.begin();
  return t;
}
