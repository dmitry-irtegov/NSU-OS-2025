CC = gcc
CFLAGS = -Wall -Wextra -std=c11

all: server client

server: server.c
	$(CC) $(CFLAGS) -o server server.c

client: client.c
	$(CC) $(CFLAGS) -o client client.c

clean:
	rm -f server client
	rm -f /tmp/unix_socket_lab30

.PHONY: all clean
