ACLOCAL_AMFLAGS = -I m4

SOMEWHERE=$(patsubst %, $(srcdir)/$(2)/%, $(1))
XML2=$(call SOMEWHERE,$(1),libxml2)
HWHEN=$(call SOMEWHERE,$(1),htmlish/html_when/src)
GENERATE=$(call SOMEWHERE,$(1),src)
CYSTUFF=$(call SOMEWHERE,$(1),cystuff/src)

XMLFIXES_C=$(srcdir)/libxmlfixes/src/libxmlfixes.c
HTMLISH_C=$(srcdir)/htmlish/src/htmlish.c
DDATE_C=$(srcdir)/ddate/ddate.c

AM_CFLAGS = -I $(srcdir)/libxml2/include -I $(srcdir)/libxml2/build/include
AM_CFLAGS += -I $(srcdir)/htmlish/html_when/src -I $(srcdir)/htmlish/src
AM_CFLAGS += -I $(srcdir)/libxmlfixes/src -I $(srcdir)/ddate

bin_PROGRAMS = generate news-log make-log describe
noinst_PROGRAMS = statements2source

ADD_TO=$(foreach file, $(3), $(eval $(1) += $(2)/$(file)))

generate_SOURCES =
$(call ADD_TO, generate_SOURCES, $(srcdir)/src, \
	main.c storygit.c repo.c create.c db.c note.c)
generate_SOURCES += o/category.gen.c $(XMLFIXES_C) $(HTMLISH_C) $(DDATE_C)

generate_CFLAGS = $(SQLITE_CFLAGS)

generate_CFLAGS += $(AM_CFLAGS)

generate_LDADD = $(srcdir)/libxml2/build/libxml2.la $(SQLITE_LIBS)
BUILT_SOURCES = o/category.gen.c

o/category.gen.c o/category.gen.h: src/categories.list ./str_to_enum_trie/main
	(cd o && noupper=1 prefix=category enum=CATEGORY exec ../$(srcdir)/str_to_enum_trie/main) <$<
./str_to_enum_trie/main: str_to_enum_trie/src/main.c
	$(MAKE) -C str_to_enum_trie main

news_log_SOURCES = $(XMLFIXES_C) $(HTMLISH_C)
$(call ADD_TO news_log_SOURCES, $(srcdir)/src,
  news-log.c repo.c)

news_log_LDADD = $(srcdir)/libxml2/build/libxml2.la $(SQLITE_LIBS)

make_log_SOURCES = make-log.c itoa.c db.c note.c become.c o/template/make-log.html.c

BUILT_SOURCES += o/template/make-log.html.c

o/template/%.c: $(srcdir)/template/% $(srcdir)/ctemplate/generate | o/template
	$(srcdir)/ctemplate/generate < $< > $@.temp
	mv $@.temp $@

o/template:
	mkdir $@

$(srcdir)/ctemplate/generate: | $(srcdir)/ctemplate/src/generate.c
	$(make) -C $(dir $@) generate

statements2source_SOURCES = o/template/statements2source.c.c

BUILT_SOURCES += o/template/statements2source.c.c

describe_SOURCES = describe.c itoa.c db.c note.c

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
