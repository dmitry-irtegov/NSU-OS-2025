CC = cc
CFLAGS = -v -mt
LIBS = -lnsl -lsocket -lresolv
SRC = main.c buffer.c
TARGET = out

OBJECTS = $(SRC:.c=.o)

$(TARGET): $(OBJECTS)
	$(CC) $^ -o $@ $(LIBS)

main.o: main.c buffer.h
	$(CC) $(CFLAGS) -c main.c -o $@

buffer.o: buffer.c buffer.h
	$(CC) $(CFLAGS) -c buffer.c -o $@

clean:
	rm -rf $(OBJECTS) $(TARGET)

.PHONY: clean
