CC = gcc
CFLAGS = -Wall -Wextra
TARGETS = server client
SOURCES = server.c client.c
OBJECTS = $(SOURCES:.c=.o)
SOCKET_PATH = /tmp/to_upper

all: $(TARGETS)

server: server.o
	$(CC) $(CFLAGS) -lsocket -o server server.o

client: client.o
	$(CC) $(CFLAGS) -lsocket -o client client.o

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(TARGETS) $(OBJECTS)
	rm -f $(SOCKET_PATH)
