CC = gcc
CFLAGS = -O2 -pthread -Wall -Wextra
# override num_steps via: make num_steps=1000000
ifneq ($(num_steps),)
CFLAGS += -Dnum_steps=$(num_steps)
endif

TARGET = pi_pthread
SRC = main.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET) *.o

.PHONY: all clean
