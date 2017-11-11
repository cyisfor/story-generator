include coolmake/top.mk

P=libgit2 sqlite3
PKG_CONFIG_PATH:=/custom/libgit2/lib/pkgconfig
export PKG_CONFIG_PATH

LIBXML:=libxml2
XMLVERSION:=include/libxml/xmlversion.h

CFLAGS+=-ggdb
CFLAGS+=-I. -Iddate/ -Ihtmlish/src -Ihtml_when/src -Ihtml_when/ -Ilibxml2/include -Ictemplate/src -Icystuff/src
LDLIBS+=-lbsd -lreadline
LDLIBS+=$(shell xml2-config --libs | sed -e's/-xml2//g')

LIBS=htmlish/libhtmlish.la \
	libxml2/libxml2.la \
	$(O)/ddate.o

$(O)/ddate.o:
	$(MAKE) -C ddate ddate.o
	mv ddate/ddate.o $@

all: generate test_git describe make-log list-commits set-censored statements2source

N=statements2source db itoa cystuff/mmapfile note
OUT=statements2source
$(eval $(PROGRAM))

N=statements2source
$(OBJ) $(DEP): o/template/statements2source.c

N=cystuff/mmapfile
$(OBJ): cystuff/src/mmapfile.c | $(O)/cystuff
	$(COMPILE)

o/cystuff: | o/
	mkdir $@

N=main storygit repo create itoa db note category.gen
OUT=generate
$(eval $(PROGRAM))

N=make-log itoa db note
OUT=make-log
$(eval $(PROGRAM))

N=make-log
$(OBJ) $(DEP): o/template/make-log.html.c o/template/make-log.row.html.c

$(O)/template/%.c: template/% ctemplate/generate | $(O)/template
	./ctemplate/generate < $< >$@.temp
	mv $@.temp $@

$(O)/template: | $(O)
	mkdir $@

$(TOP)/ctemplate/generate: | ctemplate
	$(MAKE) -C ctemplate generate

$(TOP)/ctemplate: setup

N=list-commits storygit repo note itoa
OUT=list-commits
$(eval $(PROGRAM))

N=describe itoa db note
OUT=describe
$(eval $(PROGRAM))

N=test_git storygit repo itoa db note
OUT=test_git
$(eval $(PROGRAM))

N=set-censored db note itoa
OUT=set-censored
$(eval $(PROGRAM))

$(O)/category.gen.c $(O)/category.gen.h: src/categories.list ./str_to_enum_trie/main
	(cd o && noupper=1 prefix=category enum=CATEGORY exec ../str_to_enum_trie/main)<$<

$(O)/category.gen.lo: $(O)/category.gen.c
	$(COMPILE)

N=main
$(OBJ) $(DEP): $(O)/category.gen.c $(O)/category.gen.h

./str_to_enum_trie/main: | setup
	$(MAKE) -C str_to_enum_trie main

N=db list-commits
$(OBJ) $(DEP): $(O)/db-sql.gen.c src/db_oid/gen.h db_oid/make.c

$(O)/%.sql.gen.c: src/%.sql $(O)/make-sql
	$(O)/make-sql <$< >$@.temp
	mv $@.temp $@

$(O)/%.ctempl.c: src/% ctemplate/generate
	./ctemplate/generate <$< >$@.temp
	mv $@.temp $@

src/db_oid/gen.h: $(O)/db_oid/make | src/db_oid/same.h src/db_oid/custom.h
	$< src/db_oid

N=db_oid/make
OUT=$(O)/db_oid/make
$(eval $(PROGRAM))

$(OBJ) $(DEP): | $(O)/db_oid

$(OBJ) $(DEP): db_oid/make.c

$(O)/make-sql: $(O)/make-sql.lo
	$(LINK)

$(O)/db.lo: o/db.sql.gen.c  o/indexes.sql.gen.c  o/upgrade.sql.gen.c

$(O)/db_oid: | $(O)
	mkdir $@

.PHONY: setup

$(LIBXML)/$(XMLVERSION): setup

setup:
	sh setup.sh
	$(MAKE) -C htmlish setup

htmlish/libhtmlish.la: | htmlish
	$(MAKE) -C $| $(notdir $@)

libxml2/libxml2.la: | libxml2
	$(MAKE) -C $| $(notdir $@)
