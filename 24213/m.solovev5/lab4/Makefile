all: lab4

lab4: lab4.o list.o
	cc lab4.o list.o

lab4.o: lab4.c list.h
	cc -v -c lab4.c

list.o: list.c list.h 
	cc -v -c list.c

clean:
	rm -f a.out *.o

