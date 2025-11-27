all: server client

server: server.c connection.c
	gcc -o server server.c connection.c -lsocket

client: client.c connection.c
	gcc -o client client.c connection.c -lsocket

clean:
	rm -f server client