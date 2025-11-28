#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <sys/select.h>

#include "protocol.h"

#define BUF_SIZE 4096
#define MAX_CLIENTS 255

#define CMD_OPEN  0x01
#define CMD_CLOSE 0x02

int targets[MAX_CLIENTS];

struct Parser tunnel_parser;

void send_command(int tunnel_fd, int client_id, char cmd_type) {
    char cmd_buf[2];
    cmd_buf[0] = cmd_type;
    cmd_buf[1] = (char)client_id;
    
    char encoded[32];
    int len = protocol_encode(0, cmd_buf, 2, encoded);
    write(tunnel_fd, encoded, len);
}

int connect_to_forward(const char *host, int port) {    
    struct hostent *hostent = gethostbyname(host);
    if (hostent == NULL) {
        perror("gethostbyname");
        return -1;
    }

    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("socket");
        return -1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    memcpy(&addr.sin_addr, hostent->h_addr, hostent->h_length);

    if (connect(sock_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect");
        close(sock_fd);
        return -1;
    }

    return sock_fd;
}

int setup_server(int port) {
    int sock_fd;

    if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        return -1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(sock_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(sock_fd);
        return -1;
    }

    if (listen(sock_fd, 1) < 0) {
        perror("listen");
        close(sock_fd);
        return -1;
    }

    return sock_fd;
}

void packet_received(int packet_id, const char *data, int len, int tunnel_fd) {
    if (packet_id == 0) {
        if (len < 2) return;
        unsigned char cmd = (unsigned char)data[0];
        int client_id = (unsigned char)data[1];

        if (cmd == CMD_CLOSE) {
            if (targets[client_id] != -1) {
                close(targets[client_id]);
                targets[client_id] = -1;
                printf("Closed connection for client %d\n", client_id);
            }
        }
    } else {
        if (packet_id > 0 && packet_id < MAX_CLIENTS && targets[packet_id] != -1) {
            write(targets[packet_id], data, len);
        }
    }
}

void handle_client_data(int id, int tunnel_fd) {
    char buf[BUF_SIZE];
    int bytes_read = read(targets[id], buf, BUF_SIZE);
    if (bytes_read <= 0) {
        close(targets[id]);
        targets[id] = -1;
        printf("Client %d disconnected\n", id);
        send_command(tunnel_fd, id, CMD_CLOSE);
        return;
    }

    char encoded[BUF_SIZE * 2 + 16];
    int encoded_len = protocol_encode(id, buf, bytes_read, encoded);
    write(tunnel_fd, encoded, encoded_len);
}

void handle_tunnel_data(int tunnel_fd) {
    char buf[BUF_SIZE];
    int bytes_read = read(tunnel_fd, buf, BUF_SIZE);
    if (bytes_read <= 0) {
        printf("Tunnel connection lost.\n");
        return;
    }

    for (int i = 0; i < bytes_read; i++) {
        if (protocol_decode(&tunnel_parser, buf[i])) {
            packet_received(tunnel_parser.client_id, tunnel_parser.buffer, tunnel_parser.len, tunnel_fd);
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Usage: %s <transmitter_host> <transmitter_port> <target_host:target_port>\n", argv[0]);
        return 1;
    }

    int user_port = atoi(argv[1]);
    char *receiver_host = argv[2];
    int receiver_port = atoi(argv[3]);

    parser_init(&tunnel_parser);

    for (int i = 0 ; i < 255; i++) {
        targets[i] = -1;
    }

    int listen_fd = setup_server(user_port);
    if (listen_fd < 0) {
        fprintf(stderr, "Critical: Cannot connect to Receiver\n");
        return 1;
    }
    printf("Transmitter listening on port %d...\n", user_port);

    int tunnel_fd = connect_to_forward(receiver_host, receiver_port);
    if (tunnel_fd < 0) {
        return 1;
    }

    printf("Forwarder running: Listen %d, Tunnel to %s:%d\n", listen_fd, receiver_host, receiver_port);

    fd_set main_set;

    while (1) {
        FD_ZERO(&main_set);
        FD_SET(tunnel_fd, &main_set);
        FD_SET(listen_fd, &main_set);
        int fd_max = (tunnel_fd > listen_fd) ? tunnel_fd : listen_fd;

        for (int i = 1; i < MAX_CLIENTS; i++) {
            if (targets[i] != -1) {
                FD_SET(targets[i], &main_set);
                if (targets[i] > fd_max) fd_max = targets[i];
            }
        }

        if (select(fd_max + 1, &main_set, NULL, NULL, NULL) < 0) {
            perror("select");
            break;
        }
        
        if (FD_ISSET(tunnel_fd, &main_set)) {
            handle_tunnel_data(tunnel_fd);
        }

        if (FD_ISSET(listen_fd, &main_set)) {
            int client_fd = accept(listen_fd, NULL, NULL);
            if (client_fd >= 0) {
                int new_id = -1;
                for (int i = 1; i < MAX_CLIENTS; i++) {
                    if (targets[i] == -1) {
                        new_id = i;
                        break;
                    }
                }
                if (new_id != -1) {
                    targets[new_id] = client_fd;
                    printf("New user connected! Accepted new client %d\n", new_id);
                    send_command(tunnel_fd, new_id, CMD_OPEN);
                } else {
                    printf("Rejected new client: max clients reached\n");
                    close(client_fd);
                }
            }
        }

        for (int i = 1; i < MAX_CLIENTS; i++) {
            if (targets[i] != -1 && FD_ISSET(targets[i], &main_set)) {
                handle_client_data(i, tunnel_fd);
            }
        }        
    }

    return 0;
}