CC = gcc
CFLAGS = -Wall -pthread

lab9: lab9.c
	$(CC) $(CFLAGS) lab9.c -o lab9

lab9_test: lab9_test.c
	$(CC) $(CFLAGS) lab9_test.c -o lab9_test

all: lab9 lab9_test

clean:
	rm -f lab9 lab9_test

.PHONY: clean
