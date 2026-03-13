all: lab33

lab33: lab33.o worker.o cache.o
	gcc -Wall -Wextra -Werror -o lab33 $^ -fsanitize=address

lab33.o: lab33.c
	gcc -c -Wall -Wextra -Werror lab33.c -fsanitize=address

worker.o: worker.c
	gcc -c -Wall -Wextra -Werror worker.c -fsanitize=address

cache.o: cache.c
	gcc -c -Wall -Wextra -Werror cache.c -fsanitize=address

clean:
	rm -rf lab33.o worker.o cache.o lab33
