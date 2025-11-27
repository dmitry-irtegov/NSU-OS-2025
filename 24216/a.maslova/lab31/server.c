#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <ctype.h>

#define SOCKET_PATH "/tmp/my_socket"
#define BUFFER_SIZE 1024

int create_server_socket() {
    int sfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sfd == -1) return -1;

    struct sockaddr_un addr = {0};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path)-1);

    unlink(SOCKET_PATH);
    if (bind(sfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) return -1;
    if (listen(sfd, 10) == -1) return -1;

    return sfd;
}

int main() {
    int sfd = create_server_socket();
    if (sfd == -1) { perror("server"); exit(1); }

    fd_set master, readfds;
    FD_ZERO(&master);
    FD_SET(sfd, &master);
    int maxfd = sfd;

    printf("Сервер слушает %s\n", SOCKET_PATH);

    while (1) {
        readfds = master;

        if (select(maxfd + 1, &readfds, NULL, NULL, NULL) < 0) {
            perror("select");
            continue;
        }

        for (int fd = 0; fd <= maxfd; fd++) {
            if (!FD_ISSET(fd, &readfds)) continue;

            if (fd == sfd) {
                int cfd = accept(sfd, NULL, NULL);
                if (cfd == -1) { perror("accept"); continue; }

                FD_SET(cfd, &master);
                if (cfd > maxfd) maxfd = cfd;

                printf("Клиент подключен\n");
            }
            else {
                char buf[BUFFER_SIZE];
                ssize_t n = read(fd, buf, BUFFER_SIZE);

                if (n <= 0) {
                    printf("Клиент отключен\n");
                    close(fd);
                    FD_CLR(fd, &master);
                } else {
                    for (int i = 0; i < n; i++)
                        buf[i] = toupper((unsigned char)buf[i]);

                    fwrite(buf, 1, n, stdout);
                    fflush(stdout);
                }
            }
        }
    }
}
