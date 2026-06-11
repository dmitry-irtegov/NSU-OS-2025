NAME = http_client
CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c11 -g

SRCS = main.c http_client.c terminal_mode.c url.c buffer.c pager.c
OBJS = $(SRCS:.c=.o)

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(OBJS)
