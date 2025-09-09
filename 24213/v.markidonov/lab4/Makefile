OUTFILE = out
FILES = main.c llist.c
FLAGS = -v
COMPILER = cc

all: $(FILES)
	$(COMPILER) -o $(OUTFILE) $(FILES) $(FLAGS)

run: all
	./$(OUTFILE)

clean:
	rm -f $(OUTFILE)