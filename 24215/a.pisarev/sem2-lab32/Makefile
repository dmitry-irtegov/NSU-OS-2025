CC      := gcc
CFLAGS  := -Wall -Wextra -Wpedantic -O2 -g -std=gnu11 -pthread
LDFLAGS := -pthread
headers := -Iheaders

SRCDIR  := src
INCDIR  := headers
OBJDIR  := obj

SRCS := $(wildcard $(SRCDIR)/*.c)
OBJS := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SRCS))
TARGET := proxy_threaded

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) $(headers) -c -o $@ $<

$(OBJDIR):
	mkdir -p $(OBJDIR)

clean:
	rm -rf $(OBJDIR) $(TARGET) logs

run: $(TARGET)
	./$(TARGET)
