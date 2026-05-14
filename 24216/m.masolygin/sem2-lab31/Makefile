CC = gcc
CFLAGS = -Wall -Wextra
SRCS = proxy.c http.c cache.c
TARGET = proxy

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS)

clean:
	rm -f $(TARGET)
