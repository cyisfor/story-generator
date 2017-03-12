import db

void go() {
	auto sel = db.prepare("SELECT id,location,title,description FROM stories");
	auto upd = db.prepare("UPDATE stories SET location = ?, title = ?, description = ? WHERE id = ?");
	auto t = db.transaction();
	scope(exit) {
		sel.finalize();
		upd.finalize();
		t.commit();
	}
	struct Row {
		long id;
		ubyte[] location;
		ubyte[] title;
		ubyte[] description;
	}
	while(sel.next()) {
		auto row = sel.as!Row();
		upd.go(row.location,
					 row.title,
					 row.description,
					 row.id)
	}

	sel.finalize();
	upd.finalize();

	sel = db.prepare("SELECT id,title,first_words FROM chapters");
	upd = db.prepare("UPDATE stories SET title = ?, first_words = ? WHERE id = ?");

	struct Chap {
		long id;
		ubyte[] title;
		ubyte[] first_words;
	}
	

	while(sel.next()) {
		auto row = sel.as!Chap();
		upd.go(row.title,
					 row.first_words,
					 row.id);
	}
}
