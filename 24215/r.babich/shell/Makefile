CC = gcc
TARGET = shell
SRCS = shell.c promptline.c parseline.c

$(TARGET): $(SRCS) shell.h
	$(CC) -o $(TARGET) $(SRCS)

clean:
	rm -f $(TARGET)

.PHONY: clean
