all: server client

GCC_FLAGS = -Wall -Wextra -Werror
LIBS = -lsocket -lnsl

server: server.c
	gcc $(GCC_FLAGS) server.c $(LIBS) -o server

client: client.c
	gcc $(GCC_FLAGS) client.c $(LIBS) -o client

