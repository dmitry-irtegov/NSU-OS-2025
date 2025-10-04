#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <signal.h>
#include <errno.h>

#define LISTENING_SOCKET_FILENAME "socket"
#define BUFFER_SIZE 128

void (*default_sigint_handler)(int);

void unlink_socket_file(){
	if (unlink(LISTENING_SOCKET_FILENAME)){
		perror("Failed to unlink socket file");
		_exit(-1);
	}
}

void sigint_handler(int sig){
	unlink_socket_file();
	
	signal(sig, default_sigint_handler);
	raise(sig);
}

int main(){

	int listening_socket;

	if ((listening_socket = socket(PF_UNIX, SOCK_STREAM, 0)) == -1){
		perror("Failed to create socket");
		return -1;
	}

	struct sockaddr_un sockaddr;
	sockaddr.sun_family = AF_UNIX;
	strlcpy(sockaddr.sun_path, LISTENING_SOCKET_FILENAME, sizeof(sockaddr.sun_path));

	if (unlink(LISTENING_SOCKET_FILENAME)){
		if (errno != ENOENT){
			perror("Failed to unlink already existing socket file");
			return -1;
		}
	}

	if (bind(listening_socket, (struct sockaddr*) &sockaddr, sizeof(sockaddr))){
		perror("Failed to bind socket");
		return -1;
	}


	atexit(unlink_socket_file);
	default_sigint_handler = signal(SIGINT, sigint_handler);
	if (default_sigint_handler == SIG_ERR){
		perror("Failed to set signal handler");
		return -1;
	}

	if (listen(listening_socket, 1)){
		perror("Failed to start listening");
		return -1;
	}
	
	int connection_socket;

	if ((connection_socket = accept(listening_socket, NULL, NULL)) == -1){
		perror("Failed to accept incoming connection");
		return -1;
	}

	ssize_t readed;
	char buff[BUFFER_SIZE];
	while ((readed = recv(connection_socket, buff, BUFFER_SIZE, MSG_WAITALL)) > 0){
		for (size_t i = 0; i < (size_t) readed; i++){
			buff[i] = toupper(buff[i]);
		}

		if (fwrite(buff, 1, readed, stdout) != (size_t) readed){
			fprintf(stderr, "Failed to write recived data");
			return -1;
		}
	}

	if (readed == -1){
		perror("Failed to read data from connected socket");
		return -1;
	}

	return 0;
}

