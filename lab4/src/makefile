CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -O2
AR = ar

.PHONY: all clean

all: parallel_min_max zombie process_memory psum

parallel_min_max: parallel_min_max.o find_min_max.o utils.o
	$(CC) $(CFLAGS) $^ -o $@

zombie: zombie.o
	$(CC) $(CFLAGS) $^ -o $@

process_memory: process_memory.o
	$(CC) $(CFLAGS) $^ -o $@

psum: parallel_sum.o utils.o libsum.a
	$(CC) $(CFLAGS) -pthread parallel_sum.o utils.o -L. -lsum -o $@

libsum.a: sum.o
	$(AR) rcs $@ $^

parallel_sum.o: parallel_sum.c sum.h utils.h
	$(CC) $(CFLAGS) -pthread -c $< -o $@

parallel_min_max.o: parallel_min_max.c find_min_max.h utils.h
find_min_max.o: find_min_max.c find_min_max.h utils.h
utils.o: utils.c utils.h
sum.o: sum.c sum.h

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f parallel_min_max zombie process_memory psum *.o libsum.a
