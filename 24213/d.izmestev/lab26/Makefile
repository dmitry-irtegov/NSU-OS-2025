CC = cc
CFLAGS = -v

all: lab26 toup

lab26: main.c
	$(CC) $(CFLAGS) main.c -o lab26

toup: toup.c
	$(CC) $(CFLAGS) toup.c -o toup

clean:
	rm -f lab26 toup

.PHONY: all clean
