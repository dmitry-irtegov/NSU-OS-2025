#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "connection.h"

int main(int argc, char* argv[]) {
    char buf[BUF_SIZE];
    int fd, rc;

    connection(SOCKET_ADDRES, &fd, 1);

    while ((rc = read(STDIN_FILENO, buf, sizeof(buf))) > 0) {
        int bytes_written = 0;
        while (bytes_written < rc) {
            int written = write(fd, buf + bytes_written, rc - bytes_written);
            if (written < 0) {
                if (rc > 0) {
                    fprintf(stderr, "partial write\n");
                } else {
                    perror("write error");
                    close(fd);
                    exit(1);
                }
            }
            bytes_written += written;
        }
    }
    close(fd);
    return 0;
}