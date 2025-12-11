CC = gcc
CFLAGS = -Wall -g

OBJS = shell.o parseline.o promptline.o jobs.o builtins.o
TARGET = shell

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

shell.o: shell.c shell.h jobs.h builtins.h
	$(CC) $(CFLAGS) -c shell.c

parseline.o: parseline.c shell.h
	$(CC) $(CFLAGS) -c parseline.c

promptline.o: promptline.c shell.h
	$(CC) $(CFLAGS) -c promptline.c

jobs.o: jobs.c jobs.h shell.h
	$(CC) $(CFLAGS) -c jobs.c

builtins.o: builtins.c builtins.h jobs.h shell.h
	$(CC) $(CFLAGS) -c builtins.c

# tokenizer.o: tokenizer.c tokenizer.h jobs.h shell.h
# 	$(CC) $(CFLAGS) -c tokenizer.c

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean