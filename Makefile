CFLAGS += `pkg-config --cflags wimlib` -Wall -Wextra -Werror
LIBS += `pkg-config --libs wimlib`

PREFIX := /usr/local

.PHONY: all
all: mkmedia

mkmedia: mkmedia.o
	$(CC) $(LDFLAGS) $^ -o $@ $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $^ -o $@

.PHONY: install
install: mkmedia buildiso esddl
	install mkmedia $(PREFIX)/bin/
	install esddl $(PREFIX)/bin/
	install buildiso $(PREFIX)/bin/

.PHONY: clean
clean:
	rm -f *.o mkmedia
