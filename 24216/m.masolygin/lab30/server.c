#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "connection.h"

int main(int argc, char* argv[]) {
    struct sockaddr_un addr;
    char buf[BUF_SIZE];
    int fd, cl, rc;

    connection(SOCKET_ADDRES, &fd, 0);

    while (1) {
        if ((cl = accept(fd, NULL, NULL)) == -1) {
            perror("accept error");
            continue;
        }

        while ((rc = read(cl, buf, sizeof(buf))) > 0) {
            for (int i = 0; i < rc; i++) {
                buf[i] = toupper(buf[i]);
            }
            int bytes_written = 0;
            while (bytes_written < rc) {
                int written = write(STDOUT_FILENO, buf + bytes_written,
                                    rc - bytes_written);
                if (written < 0) {
                    perror("write error");
                    close(cl);
                    close(fd);
                    exit(1);
                }
                bytes_written += written;
            }
        }
        if (rc == -1) {
            perror("read");
            close(cl);
            continue;
        } else if (rc == 0) {
            printf("EOF\n");
            close(cl);
        }
    }

    close(fd);

    return 0;
}