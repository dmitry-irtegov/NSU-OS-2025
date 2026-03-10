all: lab32

lab32: lab32.o cache.o
	gcc -Wall -Wextra -Werror -o lab32 lab32.o cache.o -lsocket -lnsl

lab32.o: lab32.c
	gcc -c -Wall -Wextra -Werror lab32.c -lsocket -lnsl

cache.o: cache.c
	gcc -c -Wall -Wextra -Werror cache.c

clean:
	rm -rf lab32.o cache.o lab32
