CC = cc
CFLAGS = -v -mt -lrt
TARGET = out

$(TARGET): main.c
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -f $(TARGET)

.PHONY: clean
