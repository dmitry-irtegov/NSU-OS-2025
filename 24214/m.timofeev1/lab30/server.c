#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <ctype.h>
#include <string.h>

#define BUFFER_SIZE 256
#define SOCKET_PATH "/tmp/mysocket"

int main()
{
	int sockfd, clientfd;
	struct sockaddr_un addr;
	char buffer[BUFFER_SIZE];

	sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sockfd == -1)
	{
		perror("failed to create socket");
		return -1;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

	unlink(SOCKET_PATH);

	if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
	{
		perror("failed to bind socket");
		close(sockfd);
		return -1;
	}

	if (listen(sockfd, 1) == -1)
	{
		perror("failed to listen");
		close(sockfd);
		unlink(SOCKET_PATH);
		return -1;
	}

	clientfd = accept(sockfd, NULL, NULL);
	if (clientfd == -1)
	{
		perror("failed to accept");
		close(sockfd);
		unlink(SOCKET_PATH);
		return -1;
	}

	ssize_t nread;
	while ((nread = read(clientfd, buffer, BUFFER_SIZE - 1)) > 0)
	{
		buffer[nread] = '\0';

		for (int i = 0; i < nread; i++)
		{
			buffer[i] = toupper((unsigned char)buffer[i]);
		}

		printf("%s", buffer);
	}

	if (nread == -1)
	{
		perror("failed to read from socket");
		close(clientfd);
		close(sockfd);
		unlink(SOCKET_PATH);
		return -1;
	}

	printf("\n");

	close(clientfd);
	close(sockfd);
	unlink(SOCKET_PATH);

	return 0;
}