CC = gcc
CFLAGS = -Wall -Wextra -pthread
SRCS = proxy.c pool.c conn.c cache.c origin.c request.c listener.c
TARGET = proxy

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS) -lsocket -lnsl

clean:
	rm -f $(TARGET)
