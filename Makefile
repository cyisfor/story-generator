P=libgit2 sqlite3
PKG_CONFIG_PATH:=/custom/libgit2/lib/pkgconfig
export PKG_CONFIG_PATH

LIBXML:=libxml2
XMLVERSION:=include/libxml/xmlversion.h

CFLAGS+=-ggdb -O2 -fdiagnostics-color=always $(shell pkg-config --cflags $(P))
CFLAGS+=-Io -Iddate/ -Ihtmlish/src -Ihtml_when/src -Ihtml_when/ -Ilibxml2/include
LDLIBS+=-lbsd $(shell pkg-config --libs $(P))
LDLIBS+=$(shell xml2-config --libs | sed -e's/-xml2//g')

derp: setup
	$(MAKE) all

all: generate test_git describe

LINK=$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

# bleah, accumulation side effects...
O=$(patsubst %,o/%.o,$N) ddate/ddate.o htmlish/libhtmlish.a\
$(foreach name,$(N),$(eval targets:=$$(targets) $(name)))
S=$(patsubst %,src/%.c,$N)

N=main storygit repo create db note
generate: $O
	$(LINK)

N=describe db note
describe: $O 
	$(LINK) -lreadline

N=test_git storygit repo db note
test_git: $O
	$(LINK)

o/%.d: src/%.c  $(LIBXML)/$(XMLVERSION) | o
	$(CC) $(CFLAGS) -MG -MM -o $@ $<

o/%.o: src/%.c | o
	$(CC) $(CFLAGS) -c -o $@ $<

o/category.gen.c o/category.gen.h: src/categories.list ./str_to_enum_trie/main
	filename=o/category.gen.T prefix=category enum=CATEGORY ./str_to_enum_trie/main <$<

./str_to_enum_trie/main:
	$(MAKE) -C str_to_enum_trie main

o/db.o: o/db-sql.gen.c src/db_oid/gen.h src/db_oid/make.c

o/db-sql.gen.c: src/db.sql o/make-sql
	./o/make-sql <$< >$@.temp
	mv $@.temp $@

src/db_oid/gen.h: o/db_oid/make | src/db_oid/same.h src/db_oid/custom.h
	cd src/db_oid && ../../o/db_oid/make

o/db_oid/make: o/db_oid/make.o
o/db_oid/make.o: | o/db_oid

o/make-sql: o/make-sql.o
	$(LINK)

ddate/ddate.o:
	$(MAKE) -C ddate ddate.o

o o/db_oid:
	mkdir $@

clean:
	rm -rf o generderp

htmlish/libhtmlish.a: descend

descend:
	$(MAKE) -C htmlish libhtmlish.a

.PHONY: descend setup

$(LIBXML)/$(XMLVERSION): descend

setup:
	sh setup.sh
	$(MAKE) -C htmlish setup

git-tools/pushcreate: git-tools/pushcreate.c
	$(MAKE) -C git-tools

push: setup git-tools/pushcreate
	[[ -n "$(remote)" ]]
	./git-tools/pushcreate "$(remote)"
	$(MAKE) -C htmlish push remote="$(remote)/htmlish"

-include $(patsubst %, o/%.d,$(targets))
