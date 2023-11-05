CC=gcc
CFLAGS=-g -pedantic -std=gnu17 -Wall -Werror -Wextra -pthread

.PHONY: all
all: nyuenc

nyuenc: nyuenc.o functions.o
	$(CC) $(CFLAGS) -o nyuenc nyuenc.o functions.o

nyuenc.o: nyuenc.c functions.h
	$(CC) $(CFLAGS) -c nyuenc.c

functions.o: functions.c functions.h
	$(CC) $(CFLAGS) -c functions.c

.PHONY: clean
clean:
	rm -f *.o nyuenc