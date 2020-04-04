PREFIX  := /usr/local

CFLAGS  := -std=c99 -pedantic -Wall -D_GNU_SOURCE -Dconst= \
	$(shell pkg-config --cflags libxml-2.0) \
	$(shell pkg-config --cflags libarchive) \
	$(shell pkg-config --cflags libcurl) \
	$(shell pkg-config --cflags wimlib)

LDFLAGS :=

LIBS    := \
	$(shell pkg-config --libs libxml-2.0) \
	$(shell pkg-config --libs libarchive) \
	$(shell pkg-config --libs libcurl) \
	$(shell pkg-config --libs wimlib)

.PHONY: all
all: esdurl mkmedia

esdurl: esdurl.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

mkmedia: mkmedia.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $^

.PHONY: install
install: esdurl mkmedia
	install esdurl $(PREFIX)/bin
	install mkmedia $(PREFIX)/bin
	install doc/esdurl.1 $(PREFIX)/man/man1/
	install doc/mkmedia.1 $(PREFIX)/man/man1/

.PHONY: clean
clean:
	rm -f *.o esdurl mkmedia
