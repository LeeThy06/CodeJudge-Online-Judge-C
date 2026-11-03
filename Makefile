CC := gcc
CFLAGS := -Wall -Wextra -O2 -g -Iinclude $(shell pkg-config --cflags gtk+-3.0 sqlite3)
GTK_SQLITE_LIBS := $(shell pkg-config --libs gtk+-3.0 sqlite3)
LDLIBS := $(GTK_SQLITE_LIBS) -pthread
COMMON := src/protocol.c

.PHONY: all clean check-deps
all: check-deps oj-server oj-client

check-deps:
	@pkg-config --exists gtk+-3.0 sqlite3 || (echo 'Missing dependencies. Install: sudo apt install build-essential gcc make pkg-config libgtk-3-dev libsqlite3-dev'; exit 1)

oj-server: src/server.c src/database.c $(COMMON) include/protocol.h include/database.h
	$(CC) $(CFLAGS) -o $@ src/server.c src/database.c $(COMMON) $(LDLIBS)

oj-client: src/client.c $(COMMON) include/protocol.h
	$(CC) $(CFLAGS) -o $@ src/client.c $(COMMON) $(LDLIBS)

clean:
	rm -f oj-server oj-client
