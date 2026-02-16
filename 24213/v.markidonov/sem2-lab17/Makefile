CC = cc
CFLAGS = -v -mt
SRC = main.c stack.c
TARGET = out

OBJECTS = $(SRC:.c=.o)

$(TARGET): $(OBJECTS)
	$(CC) $^ -o $@

main.o: main.c stack.h
	$(CC) $(CFLAGS) -c main.c -o $@

stack.o: stack.c stack.h
	$(CC) $(CFLAGS) -c stack.c -o $@

clean:
	rm -rf $(OBJECTS) $(TARGET)

.PHONY: clean
