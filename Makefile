P=libgit2
export PKG_CONFIG_PATH=/custom/libgit2/lib/pkgconfig

CFLAGS+=-ggdb -fdiagnostics-color=always $(shell pkg-config --cflags $P)
LDLIBS+=-lbsd $(shell pkg-config --libs $P)
all: generderp test_git ddate/ddate.o ddate-stub.o

LINK=$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

generderp: o/main.o o/git.o o/repo.o 
	$(LINK)
test_git: o/test_git.o o/git.o o/repo.o 
	$(LINK)

o/%.o: source/%.c | o
	$(CC) $(CFLAGS) -c -o $@ $^

ddate/ddate.o:
	$(MAKE) -C ddate ddate.o

ddate-stub.o: source/ddate-stub.c
	gcc $(CFLAGS) -Iddate -c -o $@ $^

o:
	mkdir $@
