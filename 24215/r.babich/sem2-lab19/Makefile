CC = gcc
CFLAGS = -Wall -g -pthread
TARGET = lab19

OBJ = lab19.o node.o sort.o

DEPS = node.h sort.h

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

