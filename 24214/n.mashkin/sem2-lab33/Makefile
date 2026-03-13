CC=cc
COMPILE_FLAGS=-lsocket -lnsl -mt

all: lab33

lab33: lab33.o worker.o cache.o
	$(CC) $(COMPILE_FLAGS) -o lab33 $^ 

lab33.o: lab33.c
	$(CC) -c $(COMPILE_FLAGS) lab33.c 

worker.o: worker.c
	$(CC) -c $(COMPILE_FLAGS) worker.c 

cache.o: cache.c
	$(CC) -c $(COMPILE_FLAGS) cache.c 

clean:
	rm -rf lab33.o worker.o cache.o lab33
