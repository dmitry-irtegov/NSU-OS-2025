CC = gcc
CFLAGS = -Wall -Iinclude
SRCDIR = src
OBJDIR = obj
BINDIR = .

SOURCES = $(SRCDIR)/main.c $(SRCDIR)/parseline.c $(SRCDIR)/promptline.c $(SRCDIR)/jobs.c $(SRCDIR)/pipeline.c $(SRCDIR)/signals.c $(SRCDIR)/builtins.c $(SRCDIR)/exec.c
OBJECTS = $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
TARGET = $(BINDIR)/mell

all: $(OBJDIR) $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

$(OBJDIR)/%.o: $(SRCDIR)/%.c $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR):
	mkdir -p $(OBJDIR)

clean:
	rm -rf $(OBJDIR) $(TARGET)

.PHONY: all clean
