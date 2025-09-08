OUTFILE = out
FILES = main.c llist.c
FLAGS = -Wall

all: $(FILES)
	gcc -o $(OUTFILE) $(FILES) $(FLAGS)

run: all
	./$(OUTFILE)

clean:
	rm -f $(OUTFILE)