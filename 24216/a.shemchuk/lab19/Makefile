CC = gcc
CFLAGS = -Wall -Wextra
TARGETS = match
SOURCES = match.c
OBJECTS = $(SOURCES:.c=.o)

all: $(TARGETS)
match:
	$(CC) $(CFLAGS) $(SOURCES) -o match
test:
	$(CC) $(CFLAGS) $(SOURCES) -DTEST -o match
	@./match
	@make clean
clean:
	@rm -f $(TARGETS) $(OBJECTS)	
