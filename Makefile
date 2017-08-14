P=libgit2 sqlite3
PKG_CONFIG_PATH:=/custom/libgit2/lib/pkgconfig
export PKG_CONFIG_PATH

LIBXML:=libxml2
XMLVERSION:=include/libxml/xmlversion.h

CFLAGS+=-ggdb -fdiagnostics-color=always $(shell pkg-config --cflags $(P))
CFLAGS+=-Io -Iddate/ -Ihtmlish/src -Ihtml_when/src -Ilibxml2/include
LDLIBS+=-lbsd $(shell pkg-config --libs $(P))
LDLIBS+=$(shell xml2-config --libs | sed -e's/-xml2//g')

derp: setup
	$(MAKE) all

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

o/%.o: src/%.c $(LIBXML)/$(XMLVERSION) | o
	$(CC) $(CFLAGS) -c -o $@ $<

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

setup: htmlish libxml2 html_when ddate
	$(MAKE) -C htmlish setup

libxml2: ./htmlish/html_when/libxml2
	[[ -h $@ ]] || ln -rs $< $@
./htmlish/html_when/libxml2: html_when

html_when: ./htmlish/html_when
	[[ -h $@ ]] || ln -rs $< $@
./htmlish/html_when: htmlish

define SYNC
	@if [[ ! -d $1 ]]; then \
		git clone $2 pending-$1 && \
		mv pending-$1 $1 ; \
	else \
		cd $1 && git pull; \
	fi
endef

htmlish:
#	$(call SYNC,$@,/home/code/html_when)
	$(call SYNC,$@,https://github.com/cyisfor/html_when.git)

ddate:
	$(call SYNC,$@,https://github.com/cyisfor/ddate.git)
