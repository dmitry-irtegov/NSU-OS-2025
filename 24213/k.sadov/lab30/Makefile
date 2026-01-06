all: server.out client.out

client.out: client.c
	cc -v client.c -o client.out -lsocket

server.out: server.c
	cc -v server.c -o server.out -lsocket

clean:
	rm -f server.out client.out
