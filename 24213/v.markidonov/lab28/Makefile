CC = cc
CFLAGS = -v -lgen
OUTPUT = out

$(OUTPUT): main.c
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -r $(OBJECTS) $(TARGET)

.PHONY: clean
