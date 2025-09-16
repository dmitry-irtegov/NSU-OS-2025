CC = cc
CFLAGS = -v
TARGET = lab4
OBJ = main.o list.o

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $^ -o $@

main.o: main.c list.h
	$(CC) $(CFLAGS) -c main.c -o $@

list.o: list.c list.h
	$(CC) $(CFLAGS) -c list.c -o $@

clean:
	rm -f $(TARGET) $(OBJ)
