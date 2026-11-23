CC = gcc
CFLAGS = -std=gnu11 -Wall -Wextra -O0
PROGRAMS = build/mutex_no_lock build/mutex_with_lock build/factorial build/deadlock

.PHONY: all clean

all: $(PROGRAMS)

build:
	mkdir -p $@

build/mutex_no_lock: mutex.c | build
	$(CC) $(CFLAGS) -pthread $< -o $@

build/mutex_with_lock: mutex.c | build
	$(CC) $(CFLAGS) -pthread -DUSE_MUTEX $< -o $@

build/factorial: factorial.c | build
	$(CC) $(CFLAGS) -pthread $< -o $@

build/deadlock: deadlock.c | build
	$(CC) $(CFLAGS) -pthread $< -o $@

clean:
	rm -f $(PROGRAMS)
