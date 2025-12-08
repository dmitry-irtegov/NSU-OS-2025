CC = gcc
SRCDIR = src
TARGET = shell
SRCS = $(SRCDIR)/shell.c \
			 $(SRCDIR)/promptline.c \
			 $(SRCDIR)/parser.c \
			 $(SRCDIR)/builtins.c \
			 $(SRCDIR)/job.c \
			 $(SRCDIR)/job_list.c \
			 $(SRCDIR)/process.c

$(TARGET): $(SRCS)
	$(CC) -o $@ $^

clean:
	rm -f $(TARGET)

