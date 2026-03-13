all: lab33

lab33: lab33.o worker.o cache.o
	gcc -Wall -Wextra -Werror -o lab33 $^ -lsocket -lnsl

lab33.o: lab33.c
	gcc -c -Wall -Wextra -Werror lab33.c

worker.o: worker.c
	gcc -c -Wall -Wextra -Werror worker.c

cache.o: cache.c
	gcc -c -Wall -Wextra -Werror cache.c

clean:
	rm -rf lab33.o worker.o cache.o lab33
