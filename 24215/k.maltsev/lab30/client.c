#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#define SOCKET_PATH "mysocket"
#define BUFFER_SIZE 1024

int main() {
    int sfd;
    struct sockaddr_un addr;
    char buffer[BUFFER_SIZE];
    ssize_t num_read;

    if ((sfd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket error");
        exit(1);
    }

    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (connect(sfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("connect error");
        exit(1);
    }

    printf("Connected. Type text:\n");

    while ((num_read = read(STDIN_FILENO, buffer, BUFFER_SIZE)) > 0) {
        
        char *p = buffer; 
        ssize_t remaining = num_read; 
        ssize_t written;

        while (remaining > 0) {
            written = write(sfd, p, remaining);
            
            if (written == -1) {
                if (errno == EINTR) {
                    continue;
                }
                perror("write error");
                exit(1);
            }

            p += written;
            remaining -= written;
        }
    }

    if (num_read == -1) {
        perror("read error from stdin");
        exit(1);
    }

    close(sfd);
    return 0;
}
