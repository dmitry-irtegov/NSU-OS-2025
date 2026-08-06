#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>

#define BUFF_SIZE 1024

int main() {

    int pipefd[2];
    int read_pipefd, write_pipefd;

    if (pipe(pipefd) == -1) {
        perror("pipe creation failed");
        exit(EXIT_FAILURE);
    }

    read_pipefd = pipefd[0];
    write_pipefd = pipefd[1];

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        close(write_pipefd);

        char b[BUFF_SIZE];
        ssize_t read_bytes;

        while ((read_bytes = read(read_pipefd, b, sizeof(b))) > 0) {
            for (int i = 0; i < read_bytes; i++) {
                b[i] = toupper((unsigned char)b[i]);
            }

            if (write(STDOUT_FILENO, b, read_bytes) == -1) {
                perror("write failed");
                exit(EXIT_FAILURE);
            }
        }

        if (read_bytes == -1) {
            perror("read from pipe failed");
            exit(EXIT_FAILURE);
        }

        close(read_pipefd);
        exit(EXIT_SUCCESS);
    }

    close(read_pipefd);

    char b[BUFF_SIZE];
    ssize_t read_bytes;

    while ((read_bytes = read(STDIN_FILENO, b, sizeof(b))) > 0) {
        char *msg = b;
        ssize_t rem = read_bytes;
        while (rem > 0) {
            ssize_t sent = write(write_pipefd, msg, rem);
            if (sent == -1) {
                perror("write failed");
                exit(EXIT_FAILURE);
            }
            msg += sent;
            rem -= sent;
        }
    }

    if (read_bytes == -1) {
        perror("read from stdin failed");
    }

    close(write_pipefd);

    if (wait(NULL) == -1) {
        perror("wait failed");
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}

