#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

int clients[255];



int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Usage: %s <transmitter_host> <transmitter_port> <target_host:target_port>\n", argv[0]);
        return 1;
    }

    for (int i = 0 ; i < 255; i++) {
        clients[i] = -1;
    }

    char* transmitter_host = argv[1];
    int transmitter_port = atoi(argv[2]);
    char* target = argv[3];

    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        perror("socket");
        return 1;
    }

    int tunnel_fd = connect_to_target(receiver_host, receiver_port);
    if (tunnel_fd < 0) {
        perror("connect_to_target");
        return 1;
    }

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    if (select(listen_fd + 1, &read_fds, NULL, NULL, NULL) < 0) {
        perror("select");
        return 1;
    }

    return 0;
}