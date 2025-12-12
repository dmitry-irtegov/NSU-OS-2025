CC = cc
CFLAGS = -v -D_FILE_OFFSET_BITS=64

all: a.out

a.out: main.c
	$(CC) $(CFLAGS) main.c -o $@

clean:
	rm -f a.out

.PHONY: all clean
