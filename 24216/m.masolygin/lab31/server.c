
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "connection.h"

#define MAX_CLIENTS 30

int main(int argc, char* argv[]) {
    char buf[BUF_SIZE];
    int fd, rc;
    int client_sockets[MAX_CLIENTS];
    int max_sd, sd;
    fd_set readfds;

    for (int i = 0; i < MAX_CLIENTS; i++) {
        client_sockets[i] = 0;
    }

    connection(SOCKET_ADDRES, &fd, 0);

    while (1) {
        FD_ZERO(&readfds);

        FD_SET(fd, &readfds);
        max_sd = fd;

        for (int i = 0; i < MAX_CLIENTS; i++) {
            sd = client_sockets[i];

            if (sd > 0) {
                FD_SET(sd, &readfds);
            }

            if (sd > max_sd) {
                max_sd = sd;
            }
        }

        rc = select(max_sd + 1, &readfds, NULL, NULL, NULL);

        if (rc < 0) {
            perror("select error");
            continue;
        }

        if (FD_ISSET(fd, &readfds)) {
            int new_socket;
            if ((new_socket = accept(fd, NULL, NULL)) < 0) {
                perror("accept error");
                continue;
            }

            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_sockets[i] == 0) {
                    client_sockets[i] = new_socket;
                    break;
                }
            }
        }

        for (int i = 0; i < MAX_CLIENTS; i++) {
            sd = client_sockets[i];

            if (FD_ISSET(sd, &readfds)) {
                if ((rc = read(sd, buf, sizeof(buf))) == 0) {
                    close(sd);
                    client_sockets[i] = 0;
                } else if (rc < 0) {
                    perror("read error");
                    close(sd);
                    client_sockets[i] = 0;
                } else {
                    for (int j = 0; j < rc; j++) {
                        buf[j] = toupper(buf[j]);
                    }

                    int bytes_written = 0;
                    while (bytes_written < rc) {
                        int written = write(STDOUT_FILENO, buf + bytes_written,
                                            rc - bytes_written);
                        if (written < 0) {
                            perror("write error");
                            break;
                        }
                        bytes_written += written;
                    }
                }
            }
        }
    }

    close(fd);

    return 0;
}
