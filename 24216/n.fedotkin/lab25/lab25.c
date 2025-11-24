#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/wait.h>

#define BUFFER_SIZE 31

int main() {
    int pipefd[2];

    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();

    switch (pid) {
        case -1:
            perror("Error: fork");
            exit(EXIT_FAILURE);

        case 0:
            close(pipefd[1]);

            char msg_buffer[BUFFER_SIZE];
            
            ssize_t count = read(pipefd[0], msg_buffer, BUFFER_SIZE);
            if (count == -1) {
                perror("Error: read");
                exit(EXIT_FAILURE);
            }

            for (char *ptr = msg_buffer; ptr < msg_buffer + count; ptr++) {
                *ptr = toupper((unsigned char)*ptr);
            }

            printf("Upper message: %s\n", msg_buffer);
            close(pipefd[0]);
            exit(EXIT_SUCCESS);

        default:
        
            close(pipefd[0]);
            char message[BUFFER_SIZE] = "I very love operating systems!";

            if (write(pipefd[1], message, strlen(message) + 1) == -1) {
                perror("Error: write");
                exit(EXIT_FAILURE);
            }

            close(pipefd[1]);

            if (wait(NULL) == -1) {
                perror("wait error");
                exit(EXIT_FAILURE);
            }
            exit(EXIT_SUCCESS);
    }
}