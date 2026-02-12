# Makefile for din_phil

CC = gcc
CFLAGS = -Wall -Wextra
LDLIBS = -lpthread

SRC = din_phil.c
TARGET = din_phil

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

clean:
	rm -f $(TARGET) *.o

.PHONY: all clean
