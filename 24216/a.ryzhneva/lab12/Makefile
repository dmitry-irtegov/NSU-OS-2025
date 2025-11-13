CC = gcc
CFLAGS = -Wall -g

OBJS = shell.o parseline.o promptline.o jobs.o built_in_commands.o
TARGET = shell

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

shell.o: shell.c shell.h jobs.h built_in_commands.h
	$(CC) $(CFLAGS) -c shell.c

parseline.o: parseline.c shell.h
	$(CC) $(CFLAGS) -c parseline.c

promptline.o: promptline.c shell.h
	$(CC) $(CFLAGS) -c promptline.c

jobs.o: jobs.c jobs.h shell.h
	$(CC) $(CFLAGS) -c jobs.c

built_in_commands.o: built_in_commands.c built_in_commands.h jobs.h shell.h
	$(CC) $(CFLAGS) -c built_in_commands.c

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean