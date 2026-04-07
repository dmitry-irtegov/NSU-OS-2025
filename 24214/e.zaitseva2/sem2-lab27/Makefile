CC = gcc
CFLAGS = -Wall -Wextra -O2
LDFLAGS = -lsocket -lnsl

TARGETS = server client proxy

all: $(TARGETS)

server: server.c
	$(CC) $(CFLAGS) -o $@ server.c $(LDFLAGS)

client: client.c
	$(CC) $(CFLAGS) -o $@ client.c $(LDFLAGS)

proxy: proxy.c
	$(CC) $(CFLAGS) -o $@ proxy.c $(LDFLAGS)

clean:
	rm -f $(TARGETS) *.o
