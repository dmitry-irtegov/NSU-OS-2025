
CC = gcc
CFLAGS = -Wall -Wextra -g -Ilinked_list
SRC = main.c linked_list/list.c linked_list/node.c
OBJ = $(SRC:.c=.o)
TARGET = program

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^
	rm -f $(OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean
