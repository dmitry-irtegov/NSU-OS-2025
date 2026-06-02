CC = gcc
CFLAGS = -Wall -Wextra
SRCS = http.c terminal.c url.c
TARGET = http

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS) -lsocket -lnsl

clean:
	rm -f $(TARGET)