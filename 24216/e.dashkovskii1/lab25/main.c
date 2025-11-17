#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/wait.h>

#define MSG_SIZE 256

int main() {
    char message[MSG_SIZE] = "abvGDEe zZiYkl mnOPQ RST";
    int p[2];

    if (pipe(p) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();

    switch (pid) {
        case -1:
            perror("fork");
            exit(EXIT_FAILURE);

        case 0:
            close(p[1]);

            char received_message[MSG_SIZE];
            ssize_t bytes_read = read(p[0], received_message, MSG_SIZE);
            if (bytes_read == -1) {
                perror("read");
                exit(EXIT_FAILURE);
            }

            for (ssize_t i = 0; i < bytes_read; i++) {
                received_message[i] = toupper((unsigned char)received_message[i]);
            }

            printf("Modified message: %s\n", received_message);

            close(p[0]);
            exit(EXIT_SUCCESS);

        default:
            close(p[0]);  

            ssize_t bytes_written = write(p[1], message, strlen(message) + 1);
            if (bytes_written == -1) {
                perror("write");
                exit(EXIT_FAILURE);
            }

            close(p[1]);

            if (waitpid(pid, NULL, 0) == -1) {
                perror("waitpid");
                exit(EXIT_FAILURE);
            }
            exit(EXIT_SUCCESS);
    }
}
