CC = cc
FLAGS = -v -mt

a.out: main.o node.o
	$(CC) $(FLAGS) main.o node.o

main.o: main.c node.h
	$(CC) $(FLAGS) -c main.c

node.o: node.c node.h
	$(CC) $(FLAGS) -c node.c

clean:
	rm -f a.out *.o

.PHONY: clean
