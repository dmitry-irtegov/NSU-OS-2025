#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#define BUFFER_SIZE 4096

int is_running = 1;

void signal_handler(int sig) {
    is_running = 0;
}

int main() {
    int server_fd;
    char buffer[BUFFER_SIZE];
    char* socket_path = "./upp_socket";

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket error");
        return -1;
    }

    struct sockaddr_un server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, socket_path, sizeof(server_addr.sun_path) - 1);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind error");
        close(server_fd);
        return -1;
    }

    if (listen(server_fd, 10) == -1) {
        perror("listen error");
        close(server_fd);
        unlink(socket_path);
        return -1;
    }

    printf("The server is running: socket = %s\n", socket_path);

    fd_set read_fds, cur_fds;
    FD_ZERO(&read_fds);
    FD_SET(server_fd, &read_fds);
    int mfds = server_fd;

    while (is_running) {
        cur_fds = read_fds;

        if (select(mfds + 1, &cur_fds, NULL, NULL, NULL) == -1) {
            if (errno == EINTR) {
                continue;
            }
            perror("select error");
            break;
        }

        if (FD_ISSET(server_fd, &cur_fds)) {
            int new_client = accept(server_fd, NULL, NULL);
            if (new_client == -1) {
                if (errno == EINTR) {
                    continue;
                }
                perror("accept error");
                continue;
            }
            printf("New client: fd = %d\n", new_client);

            FD_SET(new_client, &read_fds);
            if (new_client > mfds) {
                mfds = new_client;
            }
        }
        for (int fd = 0; fd <= mfds; fd++) {
            if (fd == server_fd) continue;
            if (FD_ISSET(fd, &cur_fds)) {
                int count = read(fd, buffer, BUFFER_SIZE - 1);
                if (count == 0) {
                    printf("Client %d disconnected\n", fd);
                    close(fd);
                    FD_CLR(fd, &read_fds);

                    if (fd == mfds) {
                        while (mfds > server_fd && !FD_ISSET(mfds, &read_fds)) {
                            mfds--;
                        }
                    }
                } else if (count > 0) {
                    buffer[count] = '\0';
                    for (int j = 0; j < count; j++) {
                        buffer[j] = toupper((unsigned char)buffer[j]);
                    }

                    printf("Client %d: %s", fd, buffer);
                } else {
                    perror("read error");
                    close(fd);
                    FD_CLR(fd, &read_fds);

                    if (fd == mfds) {
                        while (mfds > server_fd && !FD_ISSET(mfds, &read_fds)) {
                            mfds--;
                        }
                    }
                }
            }
        }
    }
    printf("The server is stopping.\n");

    for (int fd = 0; fd <= mfds; fd++) {
        if (fd != server_fd && FD_ISSET(fd, &read_fds)) {
            close(fd);
        }
    }

    close(server_fd);
    unlink(socket_path);
    return 0;
}
