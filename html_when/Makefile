PKG_CONFIG_PATH=/custom/gumbo/lib/pkgconfig
export PKG_CONFIG_PATH
CFLAGS+=-ggdb `pkg-config --cflags gumbo`
LDLIBS+=`pkg-config --libs gumbo`

test: libhtmlwhen.a source/app.c source/output.c
	gcc $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

libhtmlwhen.a: source/html_when.c source/selectors.c

