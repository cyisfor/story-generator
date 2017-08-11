P=libgit2 sqlite3
PKG_CONFIG_PATH:=/custom/libgit2/lib/pkgconfig
export PKG_CONFIG_PATH

CFLAGS+=-ggdb -fdiagnostics-color=always $(shell pkg-config --cflags $(P))
CFLAGS+=-Iddate/ -Ihtmlish/src -Ihtmlish/html_when/source -Ihtmlish/html_when/libxml2/include
LDLIBS+=-lbsd $(shell pkg-config --libs $(P))
LDLIBS+=$(shell xml2-config --libs | sed -e's/-xml2//g')

all: generderp test_git 

LINK=$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

O=$(patsubst %,o/%.o,$N) ddate/ddate.o htmlish/libhtmlish.a
S=$(patsubst %,source/%.c,$N)

N=main storygit repo create db
generderp: $O
	$(LINK)

N=test_git storygit repo
test_git: $O
	$(LINK)

o/%.o: source/%.c | o
	$(CC) $(CFLAGS) -c -o $@ $<

o/db.o: source/db-sql.gen.c

source/db-sql.gen.c: source/db.sql o/make-sql
	./o/make-sql <$< >$@.temp
	mv $@.temp $@

source/db-oid/gen.h: source/db-oid/base.h source/db-oid/same.h source/db-oid/custom.h o/make-db-oid
	./o/make-db-oid >$@.temp
	mv $@.temp $@

o/make-db-oid: o/db-oid/make.o
o/db-oid/make.o: o/db-oid
o/make-sql: o/make-sql.o
	$(LINK)

ddate/ddate.o:
	$(MAKE) -C ddate ddate.o

o o/db-oid:
	mkdir $@

clean:
	rm -rf o generderp

htmlish/libhtmlish.a: always
	$(MAKE) -C htmlish libhtmlish.a

.PHONY: always
