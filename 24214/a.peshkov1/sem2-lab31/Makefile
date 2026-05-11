CC = gcc
CFLAGS = -Wall -Wextra -g
LDFLAGS = -lsocket -lnsl

TARGET = proxy
OBJS = server.o http_parser.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

server.o: server.c http_parser.h
	$(CC) $(CFLAGS) -c server.c

http_parser.o: http_parser.c http_parser.h
	$(CC) $(CFLAGS) -c http_parser.c

clean:
	rm -f $(OBJS) $(TARGET)
