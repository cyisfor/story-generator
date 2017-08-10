CFLAGS+=-ggdb -fdiagnostics-color=always
LDLIBS+=-lbsd -lgit2
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
