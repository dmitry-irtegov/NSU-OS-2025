CC=gcc
CFLAGS=-std=c11 -Wall -Wextra -Wpedantic -g

CFLAGSEXTRA=

SRC=main.c
EXE=ret-code

all: $(EXE)

clean:
	rm -f $(EXE)

$(EXE): $(SRC)
	$(CC) $(CFLAGS) $(CFLAGSEXTRA) $(SRC) -o $@ 
