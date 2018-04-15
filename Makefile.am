ACLOCAL_AMFLAGS = -I m4

vpath %.c o:src:cystuff/src:libxmlfixes/src:htmlish/src
bin_PROGRAMS = generate news-log make-log describe
noinst_PROGRAMS = statements2source

generate_SOURCES = main.c storygit.c repo.c create.c itoa.c db.c note.c category.gen.c libxmlfixes.c htmlish.c

BUILT_SOURCES = o/category.gen.c

o/category.gen.c o/category.gen.h: src/categories.list ./str_to_enum_trie/main

./str_to_enum_trie/main: str_to_enum_trie/src/main.c
	$(MAKE) -C str_to_enum_trie main

news_log_SOURCES = news-log.c repo.c libxmlfixes.c htmlish.c

make_log_SOURCES = make-log.c itoa.c db.c note.c become.c o/template/make-log.html.c

BUILT_SOURCES += o/template/make-log.html.c

o/template/%.c: $(srcdir)template/% $(srcdir)/ctemplate/generate | o/template
	./ctemplate/generate < $< > $@.temp
	mv $@.temp $@

o/template:
	mkdir $@

$(srcdir)/ctemplate/generate: | $(srcdir)/ctemplate/src/generate.c
	make -C $(dir $@) generate

statements2source_SOURCES = o/template/statements2source.c.c

BUILT_SOURCES += o/template/statements2source.c.c

describe_SOURCES = describe.c itoa.c db.c note.c
