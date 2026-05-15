CC = gcc
CFLAGS = -Wall -g
TARGET = lab17

OBJ = lab17.o node.o sort.o

DEPS = node.h sort.h

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

