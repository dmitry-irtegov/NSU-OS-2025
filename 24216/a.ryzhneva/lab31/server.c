#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/poll.h>
#include <signal.h>
#include <errno.h>

#define SOCKET_PATH "/tmp/lab31_socket"
#define BUFF_SIZE 256
#define MAX_CLIENTS 1024

int listen_fd = -1;

void cleanup(int signum) {
    (void)signum;
    if (listen_fd != -1) close(listen_fd);
    unlink(SOCKET_PATH);
    printf("\nserver terminated.\n");
    exit(0);
}

int main() {
    struct sockaddr_un addr;
    struct pollfd fds[MAX_CLIENTS];
    int nfds = 1;
    char buffer[BUFF_SIZE];
    
    signal(SIGINT, cleanup);

    if ((listen_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    unlink(SOCKET_PATH);

    if (bind(listen_fd, (struct sockaddr *) &addr, sizeof(struct sockaddr_un)) == -1) {
        perror("bind failed");
        close(listen_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(listen_fd, 10) == -1) {
        perror("Listen failed");
        close(listen_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on %s. Max clients: %d\n", SOCKET_PATH, MAX_CLIENTS - 1);

    fds[0].fd = listen_fd;
    fds[0].events = POLLIN;

    while (1) {
        if (poll(fds, nfds, -1) == -1) {
            if (errno == EINTR) continue;
            perror("Poll error");
            break;
        }

        if (fds[0].revents & POLLIN) {
            int new_fd = accept(listen_fd, NULL, NULL);
            if (new_fd == -1) {
                perror("accept failed");
            } else {
                if (nfds < MAX_CLIENTS) {
                    printf("New client connected. FD: %d\n", new_fd);
                    fds[nfds].fd = new_fd;
                    fds[nfds].events = POLLIN;
                    fds[nfds].revents = 0;
                    nfds++;
                } else {
                    fprintf(stderr, "Too many clients. Connection rejected.\n");
                    close(new_fd);
                }
            }
        }

        for (int i = 1; i < nfds; i++) {
            if (fds[i].fd == -1) continue;

            if (fds[i].revents & (POLLIN | POLLERR | POLLHUP)) {
                int bytes_read = read(fds[i].fd, buffer, BUFF_SIZE);

                if (bytes_read > 0) {
                    for (int j = 0; j < bytes_read; j++) {
                        buffer[j] = toupper((unsigned char)buffer[j]);
                    }
                    if (write(STDOUT_FILENO, buffer, bytes_read) == -1) {
                        perror("write failed");
                    }
                } else {
                    printf("Client FD %d disconnected.\n", fds[i].fd);
                    close(fds[i].fd);
                    
                    fds[i] = fds[nfds - 1];
                    nfds--;
                    i--;
                }
            }
        }
    }

    cleanup(0);
    return 0;
}