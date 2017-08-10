P=libgit2
PKG_CONFIG_PATH:=/custom/libgit2/lib/pkgconfig
export PKG_CONFIG_PATH

CFLAGS+=-ggdb -fdiagnostics-color=always $(shell pkg-config --cflags $(P))
CFLAGS+=-Ihtmlish/src -Ihtmlish/html_when/source -Ihtmlish/html_when/libxml2/include
LDLIBS+=-lbsd $(shell pkg-config --libs $(P))
LDLIBS+=htmlish/libhtmlish.a
LDLIBS+=$(shell xml2-config --libs | sed -e's/-xml2//g')

all: generderp test_git ddate/ddate.o ddate-stub.o

LINK=$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

O=$(patsubst %,o/%.o,$N)
S=$(patsubst %,source/%.c,$N)

N=main git repo create
generderp: $O
	$(LINK)

N=test_git git repo
test_git: $O
	$(LINK)

o/%.o: source/%.c | o
	$(CC) $(CFLAGS) -c -o $@ $^

ddate/ddate.o:
	$(MAKE) -C ddate ddate.o

ddate-stub.o: source/ddate-stub.c
	gcc $(CFLAGS) -Iddate -c -o $@ $^

o:
	mkdir $@

clean:
	rm -rf o generderp
