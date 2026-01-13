CC = gcc
CFLAGS = -std=c11 -D__EXTENSIONS__ -Wall 
SRCDIR = src
TARGET = shell
SRCS = 	$(SRCDIR)/shell.c \
      	$(SRCDIR)/promptline.c \
        $(SRCDIR)/parser.c \
      	$(SRCDIR)/builtins.c \
        $(SRCDIR)/job.c \
        $(SRCDIR)/job_list.c \
        $(SRCDIR)/process.c \
				$(SRCDIR)/history.c

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(TARGET)
