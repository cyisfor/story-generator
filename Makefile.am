vpath %.c o:src:cystuff/src:libxmlfixes/src:htmlish/src
bin_PROGRAMS = generate news-log make-log describe

generate_SOURCES = main.c storygit.c repo.c create.c itoa.c db.c note.c category.gen.c libxmlfixes.c

BUILT_SOURCES = o/category.gen.c

o/category.gen.c o/category.gen.h: src/categories.list ./str_to_enum_trie/main

./str_to_enum_trie/main: str_to_enum_trie/src/main.c
	$(MAKE) -C str_to_enum_trie main

./str_to_enum_trie/src/main.c:
	sh setup.sh
