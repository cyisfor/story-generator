ACLOCAL_AMFLAGS = -I m4
#.SILENT:

#AM_LIBTOOLFLAGS = --quiet --no-warnings

DDATE_C=$(srcdir)/ddate/ddate.c

bin_PROGRAMS = generate news-log make-log describe make-contents ready
lib_LTLIBRARIES = libstorydb.la
noinst_PROGRAMS = statements2source db_oid/make make-sql

libstorydb_la_SOURCES = \
	src/db.c \
	src/storydb.c

libstorydb_la_LDFLAGS = $(SQLITE_LIBS) $(GIT_LIBS)
libstorydb_la_LIBADD = htmlish/libhtmlish.la
libstorydb_la_CFLAGS = $(SQLITE_CFLAGS) $(GIT_CFLAGS) \
	-I $(srcdir)/htmlish/src \
	-I $(srcdir)/libxml2/include \
	-I $(srcdir)/libxmlfixes/src \
	-I htmlish/libxmlfixes \
	-I htmlish/libxmlfixes/libxml2/include

ready_SOURCES = \
	src/ready.c \
	src/note.c \
	cystuff/src/itoa.c

ready_LDADD = libstorydb.la

ready_CFLAGS = $(libstorydb_la_CFLAGS)
ready_LDFLAGS = $(AM_LDFLAGS) $(libstorydb_la_LDFLAGS)

generate_SOURCES = \
  category.gen.c \
	ddate/ddate.c \
	src/main.c \
	src/storygit.c \
	src/repo.c \
	src/create.c \
	src/note.c \
	cystuff/src/itoa.c

generate_CFLAGS = $(libstorydb_la_CFLAGS) \
	-I $(srcdir)/ddate

generate_LDADD = libstorydb.la
generate_LDFLAGS = $(AM_LDFLAGS) $(libstorydb_la_LDFLAGS)

BUILT_SOURCES = category.gen.c category.gen.h

category.gen.c category.gen.h: src/categories.list $(srcdir)/str_to_enum_trie/main
	noupper=1 prefix=category enum=CATEGORY exec $(srcdir)/str_to_enum_trie/main <$<

$(srcdir)/str_to_enum_trie/main: str_to_enum_trie/src/main.c
	$(MAKE) -C $(srcdir)/str_to_enum_trie main

news_log_SOURCES = \
	src/news-log.c \
	src/repo.c \
	cystuff/src/become.c

news_log_CFLAGS = -I$(srcdir)/cystuff/src \
	$(libstorydb_la_CFLAGS)

news_log_LDADD = libstorydb.la
news_log_LDFLAGS =  $(libstorydb_la_LDFLAGS)

make_log_SOURCES = \
	src/make-log.c \
	src/note.c \
	cystuff/src/itoa.c \
	cystuff/src/become.c

make_log_CFLAGS = $(XMLINC) -I$(srcdir)/cystuff/src \
	$(libstorydb_la_CFLAGS)

make_log_LDADD = libstorydb.la
make_log_LDFLAGS = $(libstorydb_la_LDFLAGS)

BUILT_SOURCES += \
	template/make-log.html.c \
	template/make-log.row.html.c

template/%.c: $(srcdir)/template/% ctemplate/generate | template
	./ctemplate/generate < $< > $@.temp
	mv $@.temp $@

template:
	mkdir $@

make_contents_CFLAGS = \
	$(XMLINC) -I$(srcdir)/cystuff/src -I $(includedir)/libhtmlish/ \
	$(libstorydb_la_CFLAGS)

make_contents_SOURCES = \
	src/make-contents.c \
	src/note.c \
	cystuff/src/itoa.c
make_contents_LDADD = libstorydb.la 
make_contents_LDFLAGS = $(libstorydb_la_LDFLAGS)


BUILT_SOURCES += template/page.html.c \
	template/contents-body.html.c \
	template/contents-story.html.c

statements2source_SOURCES = \
	src/statements2source.c \
	src/db.c \
	src/note.c \
	cystuff/src/itoa.c \
	cystuff/src/mmapfile.c
statements2source_CFLAGS = $(libstorydb_la_CFLAGS) -I$(srcdir)/cystuff/src
statements2source_LDADD = htmlish/libhtmlish.la
statements2source_LDFLAGS = $(libstorydb_la_LDFLAGS)

BUILT_SOURCES += template/statements2source.c.c

describe_SOURCES = \
	src/describe.c \
	cystuff/src/itoa.c \
	src/db.c \
	src/note.c

describe_CFLAGS = $(libstorydb_la_CFLAGS)
describe_LDADD = libstorydb.la
describe_LDFLAGS = $(libstorydb_la_LDFLAGS) -lreadline

BUILT_SOURCES+=db_oid/gen.h

db_oid/gen.h: | db_oid/make
	./db_oid/make db_oid $(srcdir)/src/db_oid

db_oid_make_SOURCES = src/db_oid/make.c

%.sql.gen.c: src/%.sql make-sql
	./make-sql <$< >$@.temp
	mv $@.temp $@

BUILT_SOURCES+=schema.sql.gen.c
BUILT_SOURCES+=indexes.sql.gen.c

make_sql_SOURCES = src/make-sql.c

%.sql.stmt.c: src/%.sql.stmt statements2source
	onlydefine=1 ./statements2source derp <$< >$@.temp
	mv $@.temp $@

BUILT_SOURCES+=db.sql.stmt.c

SUBDIRS = htmlish ctemplate
