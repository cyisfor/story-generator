P=libgit2 sqlite3
PKG_CONFIG_PATH:=/custom/libgit2/lib/pkgconfig
export PKG_CONFIG_PATH

LIBXML:=htmlish/html_when/libxml2/include

CFLAGS+=-ggdb -fdiagnostics-color=always $(shell pkg-config --cflags $(P))
CFLAGS+=-Iddate/ -Ihtmlish/src -Ihtmlish/html_when/src -I$(LIBXML)
LDLIBS+=-lbsd $(shell pkg-config --libs $(P))
LDLIBS+=$(shell xml2-config --libs | sed -e's/-xml2//g')

all: generderp test_git 

LINK=$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

O=$(patsubst %,o/%.o,$N) ddate/ddate.o htmlish/libhtmlish.a
S=$(patsubst %,src/%.c,$N)

N=main storygit repo create db note
generderp: $O
	$(LINK)

N=test_git storygit repo db note
test_git: $O
	$(LINK)

o/%.o: src/%.c | o
	$(CC) $(CFLAGS) -c -o $@ $<

o/db.o: src/db-sql.gen.c src/db_oid/gen.h src/db_oid/make.c

src/db-sql.gen.c: src/db.sql o/make-sql
	./o/make-sql <$< >$@.temp
	mv $@.temp $@

src/db_oid/gen.h: o/db_oid/make | src/db_oid/same.h src/db_oid/custom.h
	./o/db_oid/make

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

htmlish/libhtmlish.a: always
	$(MAKE) -C htmlish libhtmlish.a

.PHONY: always

$(LIBXML)/xmlversion.h:
	make -C htmlish

N=db main test_parse
$O: $(LIBXML)/xmlversion.h
