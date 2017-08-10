P=libgit2
PKG_CONFIG_PATH:=/custom/libgit2/lib/pkgconfig
export PKG_CONFIG_PATH

CFLAGS+=-ggdb -fdiagnostics-color=always $(shell pkg-config --cflags $(P))
CFLAGS+=-Ihtmlish/src -Ihtmlish/html_when/source -Ihtmlish/html_when/libxml2/include
LDLIBS+=-lbsd $(shell pkg-config --libs $(P))
LDLIBS+=$(shell xml2-config --libs | sed -e's/-xml2//g')

all: generderp test_git 

LINK=$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

O=$(patsubst %,o/%.o,$N) ddate/ddate.o htmlish/libhtmlish.a
S=$(patsubst %,source/%.c,$N)

N=main git repo create
generderp: $O
	$(LINK)

N=test_git git repo
test_git: $O
	$(LINK)

o/%.o: source/%.c | o
	$(CC) $(CFLAGS) -c -o $@ $^

source/db.o: source/db-sql.gen.c

source/db-sql.gen.c: source/db.sql make-sql
	./make-sql <$< >$@.temp
	mv $@.temp $@

N=make-sql
make-sql: $O
	$(LINK)

ddate/ddate.o:
	$(MAKE) -C ddate ddate.o

o:
	mkdir $@

clean:
	rm -rf o generderp

htmlish/libhtmlish.a:
	$(MAKE) -C htmlish libhtmlish.a

.PHONY: htmlish/libhtmlish.a
