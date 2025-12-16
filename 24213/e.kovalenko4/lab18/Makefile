CC = cc
CFLAGS = -v -D_FILE_OFFSET_BITS=64
OUTPUT = a.out

$(OUTPUT): lab18.c
	$(CC) $(CFLAGS) lab18.c -o $(OUTPUT)

clean:
	rm -f $(OUTPUT)

.PHONY: clean
