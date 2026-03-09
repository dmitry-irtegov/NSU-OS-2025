all: lab31

lab31: lab31.o cache.o
	gcc -Wall -Wextra -Werror -o lab31 lab31.o cache.o -lsocket -lnsl

lab31.o: lab31.c
	gcc -c -Wall -Wextra -Werror lab31.c -lsocket -lnsl

cache.o: cache.c
	gcc -c -Wall -Wextra -Werror cache.c

clean:
	rm -rf lab31.o cache.o lab31
