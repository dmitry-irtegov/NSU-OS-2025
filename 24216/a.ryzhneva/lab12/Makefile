CC = gcc
CFLAGS = -Wall -g

OBJS = shell.o parseline.o promptline.o
TARGET = shell

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

shell.o: shell.c shell.h
	$(CC) $(CFLAGS) -c shell.c

parseline.o: parseline.c shell.h
	$(CC) $(CFLAGS) -c parseline.c

promptline.o: promptline.c shell.h
	$(CC) $(CFLAGS) -c promptline.c

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
