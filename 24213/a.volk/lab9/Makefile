CC=gcc
CFLAGS=-std=c11 -Wall -Wextra -Wpedantic
SRC=main.c

all: launch_nowait launch_wait

clean:
	rm -f launch_nowait launch_wait

launch_wait: $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $@ 

launch_nowait: $(SRC)
	$(CC) $(CFLAGS) -DNOWAIT $(SRC) -o $@ 
