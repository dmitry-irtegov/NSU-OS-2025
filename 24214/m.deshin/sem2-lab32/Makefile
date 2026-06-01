CC = gcc
CFLAGS = -Wall -Wextra -Werror -pthread
LDLIBS = -lsocket -lnsl -pthread

TARGET = proxy
SRCS = main.c buffer.c http.c net.c cache.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET) $(LDLIBS)

%.o: %.c proxy.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
