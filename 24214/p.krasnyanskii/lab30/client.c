#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <netdb.h>

int main(int argc, char** argv){
        if (argc != 4){
                printf("usage: %s hostname port text\n", argv[0]);
                return EXIT_FAILURE;
        }

        int server_sock_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_sock_fd == -1){
                perror("Socket creation error");
        }

        struct hostent* server;
        server = gethostbyname(argv[1]);
        if (!server){
                perror("gethostbyname");
                return EXIT_FAILURE;
        }
        struct sockaddr_in server_addr = { AF_INET, htons(atoi(argv[2])), };
        bcopy((char *)server->h_addr, (char *)&server_addr.sin_addr.s_addr, server->h_length);

        if (connect(server_sock_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1){
                perror("connect");
                return EXIT_FAILURE;
        }

        write(server_sock_fd, argv[3], strlen(argv[3]));
        write(server_sock_fd, "\n", 1);
        if(close(server_sock_fd)) {perror("socket close");}
}

