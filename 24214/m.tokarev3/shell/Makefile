CC = gcc

CFLAGS = -g -Wall -I$(INCDIR)
LDFLAGS =
LIBS = -lreadline

RM = rm -f
RMDIR = rm -rf
MKDIR_P = mkdir -p

SRCDIR = src
INCDIR = src/includes
OBJDIR = obj
BINDIR = bin

TARGET = $(BINDIR)/shell

SOURCES = \
    $(SRCDIR)/shell.c \
    $(SRCDIR)/builtin.c \
	$(SRCDIR)/jobs.c \
	$(SRCDIR)/execute.c \
    $(SRCDIR)/parseline.c \
    $(SRCDIR)/signals.c

OBJECTS = $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@$(MKDIR_P) $(BINDIR)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@$(MKDIR_P) $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	@$(RMDIR) $(OBJDIR) $(BINDIR)
