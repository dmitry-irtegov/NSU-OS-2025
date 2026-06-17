CC = cc
CFLAGS = -v -mt
SRC = main.c list.c
TARGET = lab20

OBJECTS = $(SRC:.c=.o)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $^ -o $@

main.o: main.c list.h
	$(CC) $(CFLAGS) -c main.c -o $@

list.o: list.c list.h
	$(CC) $(CFLAGS) -c list.c -o $@

clean:
	rm -rf $(OBJECTS) $(TARGET)

.PHONY: clean
