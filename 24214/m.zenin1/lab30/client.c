#include <sys/socket.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>

#define SERVER_SOCKET_FILENAME "socket"

int main(){

	int sock;
	if ((sock = socket(PF_UNIX, SOCK_STREAM, 0)) == -1){
		perror("Failed to create socket");
		return -1;
	}

	struct sockaddr_un sockaddr;
	sockaddr.sun_family = AF_UNIX;
	strlcpy(sockaddr.sun_path, SERVER_SOCKET_FILENAME, sizeof(sockaddr.sun_path));

	if (connect(sock, (struct sockaddr*) &sockaddr, sizeof(sockaddr))){
		perror("Failed to connect to server");
		return -1;
	}

	char msg[] = "Hello, dear SERVER, I'am very pleased to send you this message, please output it all to your stdout or I will cry loudly :(\n";

	if (write(sock, msg, sizeof(msg)) != sizeof(msg)){
		perror("Failed to send message to server, you can start crying rn");
		return -1;
	}

	return 0;
}

