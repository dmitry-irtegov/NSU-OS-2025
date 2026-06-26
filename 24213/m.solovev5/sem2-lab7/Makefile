CC = cc
FLAGS = -mt -D_POSIX_PTHREAD_SEMATICS -D_FILE_OFFSET_BITS=64 -v
TARGET = a.out

.PHONY: clean

$(TARGET): main.o utils.o
	$(CC) $(FLAGS) $^ -o $@

main.o: main.c utils.h
	$(CC) $(FLAGS) -c $< -o $@

utils.o: utils.c utils.h
	$(CC) $(FLAGS) -c $< -o $@

clean:
	rm -f *.o $(TARGET)
