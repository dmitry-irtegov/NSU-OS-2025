
CC      := gcc
CFLAGS  := -Wall -Wextra -Wpedantic -O2 -g -std=c11
LDFLAGS := 
headers := -Iheaders

SRCDIR  := src
INCDIR  := headers
OBJDIR  := obj

SRCS := $(wildcard $(SRCDIR)/*.c)
OBJS := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SRCS))
TARGET := proxy

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) $(headers) -c -o $@ $<

$(OBJDIR):
	mkdir -p $(OBJDIR)

clean:
	rm -rf $(OBJDIR) $(TARGET)

run: $(TARGET)
	./$(TARGET)

$(OBJDIR)/main.o: $(INCDIR)/common.h $(INCDIR)/event_loop.h $(INCDIR)/cache.h $(INCDIR)/utils.h
$(OBJDIR)/cache.o: $(INCDIR)/cache.h $(INCDIR)/client.h $(INCDIR)/utils.h
$(OBJDIR)/client.o: $(INCDIR)/client.h $(INCDIR)/cache.h $(INCDIR)/upstream.h $(INCDIR)/utils.h
$(OBJDIR)/upstream.o: $(INCDIR)/upstream.h $(INCDIR)/cache.h $(INCDIR)/client.h $(INCDIR)/utils.h
$(OBJDIR)/utils.o: $(INCDIR)/utils.h
$(OBJDIR)/event_loop.o: $(INCDIR)/event_loop.h $(INCDIR)/client.h $(INCDIR)/upstream.h $(INCDIR)/utils.h
