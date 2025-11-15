#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>

#define SOCKET_PATH "/tmp/mysocket"

int main()
{
	int sockfd;
	struct sockaddr_un addr;
	const char *message = "Hello, World! This is a TeSt.\n";

	sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sockfd == -1)
	{
		perror("failed to create socket");
		return -1;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

	if (connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
	{
		perror("failed to connect");
		close(sockfd);
		return -1;
	}

	ssize_t nwritten = write(sockfd, message, strlen(message) + 1);
	if (nwritten == -1)
	{
		perror("failed to write to socket");
		close(sockfd);
		return -1;
	}

	close(sockfd);

	return 0;
}