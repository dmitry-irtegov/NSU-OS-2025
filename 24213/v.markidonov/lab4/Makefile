CC = cc
CFLAGS = -v
SRC = main.c queue.c
TARGET = a.out

OBJECTS = $(SRC:.c=.o)

$(TARGET): $(OBJECTS)
	$(CC) $^ -o $@

main.o: main.c queue.h
	$(CC) $(CFLAGS) -c main.c -o $@

queue.o: queue.c queue.h
	$(CC) $(CFLAGS) -c queue.c -o $@

clean:
	rm -rf $(OBJECTS) $(TARGET)

.PHONY: clean
