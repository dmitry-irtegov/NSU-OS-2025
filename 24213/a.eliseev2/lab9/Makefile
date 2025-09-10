CC = gcc
CFLAGS = -Wall

all: fork_wait.out fork_nowait.out

clean:
	rm fork_wait.out
	rm fork_nowait.out

fork_wait.out: fork_wait.c
	$(LINK.c) fork_wait.c -o $@

fork_nowait.out: fork_nowait.c
	$(LINK.c) fork_nowait.c -o $@
