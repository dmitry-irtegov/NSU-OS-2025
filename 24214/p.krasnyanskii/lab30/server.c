#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

int main(int argc, char** argv){
        if (argc != 2){
                printf("usage: %s port_number\n", argv[0]);
                return EXIT_FAILURE;
        }

        int server_sockfd = socket(AF_INET, SOCK_STREAM, 0);

        if (server_sockfd == -1){
                perror("Socket creation error");
        }

        struct sockaddr_in server_addr = { AF_INET, htons(atoi(argv[1])), INADDR_ANY };

        if (bind(server_sockfd, (struct sockaddr*) &server_addr, sizeof(server_addr)) == -1){
                perror("Bind failed");
                return EXIT_FAILURE;
        }


        if (listen(server_sockfd, 10)){
                perror("listen");
                return EXIT_FAILURE;
        }

         struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int client_sock_fd = accept(server_sockfd, (struct sockaddr*) &client_addr, &client_addr_len);
        if (client_sock_fd == -1){
                perror("accept");
                return EXIT_FAILURE;
        }

        char buf;
        while (read(client_sock_fd, &buf, 1) > 0){
                buf = toupper(buf);
                write(STDOUT_FILENO, &buf, 1);
        }

        if (close(client_sock_fd)) {perror("client socket close");}
        if (close(server_sockfd)) {perror("client socket close");}
}

