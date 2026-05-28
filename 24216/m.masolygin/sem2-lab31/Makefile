CC = gcc
CFLAGS = -Wall -Wextra
SRCS = proxy.c http.c cache.c
TARGET = proxy

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS) -lsocket -lnsl

clean:
	rm -f $(TARGET)
