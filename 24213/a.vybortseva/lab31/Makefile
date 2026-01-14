.PHONY: all clean

all: server client

clean:
	rm -f server client

client: client31.c
	cc -v client31.c -lnsl -lsocket -o client

server: server31.c
	cc -v server31.c -lnsl -lsocket -o server
