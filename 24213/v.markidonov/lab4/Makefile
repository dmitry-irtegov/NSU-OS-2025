CFLAGS = -v
SRC = main.c llist.c
OBJS = $(SRC:.c=.o)
TARGET = out

all: $(TARGET)

$(TARGET): $(OBJS)
	$(LINK.c) -o $(TARGET) $(OBJS)

%.o: %.c
	$(COMPILE.c) $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
