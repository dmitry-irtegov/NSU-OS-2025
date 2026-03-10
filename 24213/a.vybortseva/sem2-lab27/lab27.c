#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <poll.h>
#include <errno.h>
#include <signal.h>
#include <netdb.h>

#define BACKLOG 10
#define BUF_SIZE 4096
int is_running = 1;

void signal_handler(int sig) {
    is_running = 0;
}

int close_connection(int fd, struct pollfd* fds, int nfds, int* pair_map) {
    int target = pair_map[fd];

    if (fd != -1) close(fd);
    if (target != -1) close(target);

    if (fd != -1 && fd < 1024) pair_map[fd] = -1;
    if (target != -1 && target < 1024) pair_map[target] = -1;

    for (int i = 0; i < nfds; i++) {
        if (fds[i].fd == fd || fds[i].fd == target) {
            for (int j = i; j < nfds - 1; j++) {
                fds[j] = fds[j + 1];
            }
            nfds--;
            i--;
        }
    }
    return nfds;
}

int connectToN(const char* host, int port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        perror("socket error");
        return -1;
    }

    struct hostent *server = gethostbyname(host);
    if (server == NULL) {
        herror("gethostbyname error");
        close(fd);
        return -1;
    }

     struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    memcpy(&addr.sin_addr.s_addr, server->h_addr_list[0], server->h_length);

    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("connect error");
        return -1;
    }
    return fd;
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        fprintf(stderr, "incorrect number of arguments");
        exit(1);
    }

    int portP = atoi(argv[1]);
    char* hostN = argv[2];
    int portPserv = atoi(argv[3]);

    signal(SIGINT, signal_handler);

    int server_fd;
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket error");
        exit(1);
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(portP);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind error");
        exit(1);
    }

    if (listen(server_fd, BACKLOG) != 0) {
        perror("listen error");
        exit(1);
    }

    struct pollfd fds[1024];
    memset(fds, 0, sizeof(fds));
    fds[0].fd = server_fd;
    fds[0].events = POLLIN;
    int nfds = 1;

    int pair_map[1024];
    for(int i = 0; i < 1024; i++) {
        pair_map[i] = -1;
    }

    while (is_running) {
        if (poll(fds, nfds, -1) == -1) {
            perror("poll error");
            continue;
        }

        for (int i = 0; i < nfds; i++) {
            if (fds[i].fd == -1) continue;

            if (fds[i].revents & POLLIN) {
                if (i == 0) {
                    int client_fd;
                    if ((client_fd = accept(server_fd, NULL, NULL)) == -1) { // нужен ли listen
                        perror("select error");
                        continue;
                    }

                    int fdN = connectToN(hostN, portPserv);
                    if (fdN == -1) {
                        close(client_fd);
                        continue;
                    }

                    if (nfds + 2 >= 1020) {
                        fprintf(stderr, "limit fd reached");
                        close(client_fd);
                        close(fdN);
                        continue;
                    }

                    fds[nfds].fd = client_fd;
                    fds[nfds].events = POLLIN;
                    nfds++;

                    fds[nfds].fd = fdN;
                    fds[nfds].events = POLLIN;
                    nfds++;

                    pair_map[client_fd] = fdN;
                    pair_map[fdN] = client_fd;

                } else {
                    char buffer[BUF_SIZE];
                    int count = read(fds[i].fd, buffer, BUF_SIZE);
                    if (count == 0) {
                        nfds = close_connection(fds[i].fd, fds, nfds, pair_map);
                        i--;
                    } else if (count == -1) {
                        perror("read error");
                        nfds = close_connection(fds[i].fd, fds, nfds, pair_map);
                        i--;
                    } else {
                        int target_fd = pair_map[fds[i].fd];
                        if (write(target_fd, buffer, count) == -1) {
                            perror("write error");
                            nfds = close_connection(fds[i].fd, fds, nfds, pair_map);
                            i--;
                        }
                    }
                }
            }
        }
    }
    close(server_fd);

    exit(0);
}
