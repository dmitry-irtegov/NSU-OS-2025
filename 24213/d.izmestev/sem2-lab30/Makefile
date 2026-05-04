CC = cc
CFLAGS = -v -mt
LIBS = -lsocket -lnsl -lpthread
TARGET = laba.out
SRC = main.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LIBS)

clean:
	rm -f $(TARGET)

.PHONY: all clean
