BUILT_SOURCES =

ACLOCAL_AMFLAGS = -I m4

XMLFIXES_C=$(srcdir)/libxmlfixes/src/libxmlfixes.c
DDATE_C=$(srcdir)/ddate/ddate.c

XMLINC=-I$(srcdir)/libxml2/include -I$(srcdir)/libxml2/build/include
HISHINC=-I $(srcdir)/htmlish/html_when/src -I $(srcdir)/htmlish/src -I $(srcdir)/libxmlfixes/src 

bin_PROGRAMS = generate news-log make-log describe make-contents
lib_LIBRARIES = libstorydb.la
noinst_PROGRAMS = statements2source db_oid/make o/make-sql

HISH_libs = \
	$(srcdir)/libxml2/build/libxml2.la \
	$(srcdir)/htmlish/libhtmlish.la \
	$(srcdir)/htmlish/html_when/libhtmlwhen.la

HISH_cflags = $(XMLINC) $(HISHINC)

libstorydb_la_SOURCES = \
	src/db.c \
	src/storydb.c

libstorydb_la_LIBADD = \
	$(SQLITE_LIBS) $(GIT_LIBS) \
	$(HISH_libs)

libstorydb_la_CFLAGS = $(SQLITE_CFLAGS) $(GIT_CFLAGS) $(HISH_cflags) 

generate_SOURCES = \
	src/main.c \
	src/storygit.c \
	src/repo.c \
	src/create.c \
	src/note.c

generate_SOURCES += o/category.gen.c $(XMLFIXES_C) $(DDATE_C)

generate_SOURCES += cystuff/src/itoa.c

generate_CFLAGS = $(XMLINC) $(HISHINC) \
	$(libstorydb_la_CFLAGS) \
	-I $(srcdir)/ddate

generate_LDADD = libstorydb.la

BUILT_SOURCES += o/category.gen.c

o/category.gen.c o/category.gen.h: src/categories.list ./str_to_enum_trie/main
	(cd o && noupper=1 prefix=category enum=CATEGORY exec ../$(srcdir)/str_to_enum_trie/main) <$<
./str_to_enum_trie/main: str_to_enum_trie/src/main.c
	$(MAKE) -C str_to_enum_trie main

news_log_SOURCES = $(XMLFIXES_C) \
	src/news-log.c \
	src/repo.c \
	cystuff/src/become.c

news_log_CFLAGS = -I$(srcdir)/cystuff/src \
	$(XMLINC) $(HISHINC) \
	$(libstorydb_la_CFLAGS)

news_log_LDADD = libstorydb.la

make_log_SOURCES = \
	src/make-log.c \
	src/note.c \
	cystuff/src/itoa.c \
	cystuff/src/become.c

make_log_CFLAGS = $(XMLINC) -I$(srcdir)/cystuff/src \
	$(HISHINC) \
	$(libstorydb_la_CFLAGS)
make_log_LDADD = libstorydb.la

BUILT_SOURCES += \
	o/template/make-log.html.c \
	o/template/make-log.row.html.c

o/template/%.c: $(srcdir)/template/% $(srcdir)/ctemplate/generate | o/template
	$(srcdir)/ctemplate/generate < $< > $@.temp
	mv $@.temp $@

o/template:
	mkdir $@

$(srcdir)/ctemplate/generate: | $(srcdir)/ctemplate/src/generate.c
	$(make) -C $(dir $@) generate

make_contents_CFLAGS = \
	$(XMLINC) -I$(srcdir)/cystuff/src $(HISHINC) \
	$(libstorydb_la_CFLAGS)

make_contents_SOURCES = \
	src/make-contents.c \
	src/note.c \
	cystuff/src/itoa.c
make_contents_LDADD = libstorydb.la


BUILT_SOURCES += o/template/page.html.c \
	o/template/contents-body.html.c \
	o/template/contents-story.html.c

statements2source_SOURCES = \
	src/statements2source.c \
	src/db.c \
	src/note.c \
	cystuff/src/itoa.c \
	cystuff/src/mmapfile.c
statements2source_CFLAGS = $(libstorydb_la_CFLAGS) -I$(srcdir)/cystuff/src
statements2source_LDADD = $(libstorydb_la_LIBADD)

BUILT_SOURCES += o/template/statements2source.c.c

describe_SOURCES = \
	src/describe.c \
	cystuff/src/itoa.c \
	src/db.c \
	src/note.c

describe_CFLAGS = $(XMLINC) $(HISHINC)
describe_LDADD = \
	$(SQLITE_LIBS) $(GIT_LIBS) -lreadline \
	$(srcdir)/htmlish/libhtmlish.la \
	$(srcdir)/libxml2/build/libxml2.la


define make_sublib
$(srcdir)/$(name)/build/$(target): | $(srcdir)/$(name)/build/Makefile 
	$(MAKE) -C $(srcdir)/$(name)/build

$(srcdir)/$(name)/build/Makefile: $(srcdir)/$(name)/configure | $(srcdir)/$(name)/build
	(cd $(srcdir)/$(name)/build; . ../$(srcdir)/$(name)/configure)

$(srcdir)/$(name)/build: | $(srcdir)/$(name)
	mkdir $$@

$(srcdir)/$(name)/configure: | $(srcdir)/$(name)/configure.ac
	(cd $(srcdir)/$(name); . ./autogen.sh)
endef

name=libxml2
target=libxml2.la
$(eval $(make_sublib))

BUILT_SOURCES+=db_oid/gen.h

db_oid/gen.h: db_oid/make
	./db_oid/make db_oid $(srcdir)/src/db_oid

db_oid_make_SOURCES = src/db_oid/make.c

BUILT_SOURCES+=o/schema.sql.gen.c
BUILT_SOURCES+=o/indexes.sql.gen.c

o/%.sql.gen.c: $(srcdir)/src/%.sql o/make-sql
	o/make-sql <$< >$@.temp
	mv $@.temp $@

o_make_sql_SOURCES = src/make-sql.c


o/%.sql.c: $(srcdir)/src/%.sql o statements2source
	onlydefine=1 ./statements2source derp <$< >$@.temp
	mv $@.temp $@

BUILT_SOURCES+=o/db.sql.c
