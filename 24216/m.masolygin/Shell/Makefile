

CC = gcc
CFLAGS = -Wall -g
SRCS = parseline.c promptline.c shell.c utils.c
OBJS = $(SRCS:.c=.o)
TARGET = shell

.PHONY: all debug clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

%.o: %.c shell.h
	$(CC) $(CFLAGS) -c $< -o $@

debug: CFLAGS += -DDEBUG
debug: clean all

clean:
	rm -f $(OBJS) $(TARGET)

