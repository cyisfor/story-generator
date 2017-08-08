CFLAGS+=-ggdb

test: libhtmlwhen.a source/app.c source/output.c
	gcc -o $@ $^

libhtmlwhen.a: source/html_when.c source/selectors.c

