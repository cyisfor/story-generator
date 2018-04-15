BUILT_SOURCES =

ACLOCAL_AMFLAGS = -I m4

SOMEWHERE=$(patsubst %, $(srcdir)/$(2)/%, $(1))
XML2=$(call SOMEWHERE,$(1),libxml2)
HWHEN=$(call SOMEWHERE,$(1),htmlish/html_when/src)
GENERATE=$(call SOMEWHERE,$(1),src)
CYSTUFF=$(call SOMEWHERE,$(1),cystuff/src)

XMLFIXES_C=$(srcdir)/libxmlfixes/src/libxmlfixes.c
DDATE_C=$(srcdir)/ddate/ddate.c

XMLINC=-I$(srcdir)/libxml2/include -I$(srcdir)/libxml2/build/include
HISHINC=-I $(srcdir)/htmlish/html_when/src -I $(srcdir)/htmlish/src -I $(srcdir)/libxmlfixes/src 

bin_PROGRAMS = generate news-log make-log describe
noinst_PROGRAMS = statements2source db_oid/make o/make-sql

generate_SOURCES = \
	src/main.c \
	src/storygit.c \
	src/repo.c \
	src/create.c \
	src/db.c \
	src/note.c

generate_SOURCES += o/category.gen.c $(XMLFIXES_C) $(DDATE_C)

generate_SOURCES += cystuff/src/itoa.c

generate_CFLAGS = $(SQLITE_CFLAGS) $(GIT_CFLAGS)

generate_CFLAGS += $(XMLINC) $(HISHINC) \
	-I $(srcdir)/ddate


generate_LDADD = $(srcdir)/libxml2/build/libxml2.la \
	$(SQLITE_LIBS) $(GIT_LIBS) \
	$(srcdir)/htmlish/libhtmlish.la \
	$(srcdir)/htmlish/html_when/libhtmlwhen.la

BUILT_SOURCES += o/category.gen.c

o/category.gen.c o/category.gen.h: src/categories.list ./str_to_enum_trie/main
	(cd o && noupper=1 prefix=category enum=CATEGORY exec ../$(srcdir)/str_to_enum_trie/main) <$<
./str_to_enum_trie/main: str_to_enum_trie/src/main.c
	$(MAKE) -C str_to_enum_trie main

news_log_SOURCES = $(XMLFIXES_C) \
	src/news-log.c \
	src/repo.c \
	cystuff/src/become.c

news_log_CFLAGS = -I$(srcdir)/cystuff/src $(XMLINC) $(HISHINC)

news_log_LDADD = $(srcdir)/libxml2/build/libxml2.la \
	$(GIT_LIBS) \
	$(srcdir)/htmlish/libhtmlish.la \
	$(srcdir)/htmlish/html_when/libhtmlwhen.la

make_log_SOURCES = \
	src/make-log.c \
	src/db.c \
	src/note.c \
	cystuff/src/itoa.c \
	cystuff/src/become.c

make_log_CFLAGS = $(XMLINC) -I$(srcdir)/cystuff/src $(HISHINC)
make_log_CFLAGS += $(SQLITE_CFLAGS)
make_log_LDADD = $(srcdir)/libxml2/build/libxml2.la \
	$(SQLITE_LIBS) $(GIT_LIBS) \
	$(srcdir)/htmlish/libhtmlish.la \
	$(srcdir)/htmlish/html_when/libhtmlwhen.la

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

statements2source_SOURCES = o/template/statements2source.c.c

BUILT_SOURCES += o/template/statements2source.c.c

describe_SOURCES = \
	src/describe.c \
	cystuff/src/itoa.c \
	src/db.c \
	src/note.c

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

BUILT_SOURCES+=o/db.sql.gen.c
BUILT_SOURCES+=o/indexes.sql.gen.c

o/%.sql.gen.c: $(srcdir)/src/%.sql o/make-sql
	o/make-sql <$< >$@.temp
	mv $@.temp $@

o_make_sql_SOURCES = src/make-sql.c

