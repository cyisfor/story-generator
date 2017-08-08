PKG_CONFIG_PATH=/custom/gumbo/lib/pkgconfig
export PKG_CONFIG_PATH
CFLAGS+=-ggdb `pkg-config --cflags gumbo`
LDLIBS+=`pkg-config --libs gumbo`

test: source/app.c source/output.c libhtmlwhen.a
	gcc $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

libhtmlwhen.a: o/html_when.o o/selectors.o | o
	$(AR) $(ARFLAGS) $@ $^

o:
	mkdir $@

o/%.o: source/%.c | o
	gcc $(CFLAGS) -c -o $@ $^
