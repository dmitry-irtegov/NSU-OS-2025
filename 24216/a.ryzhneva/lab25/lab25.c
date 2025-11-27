#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#define BUFF_SIZE 256

int main() {
    int pipefd[2];
    pid_t pid;

    if (pipe(pipefd) == -1) {
        perror("pipe failed");
        exit(EXIT_FAILURE);
    }

    pid = fork();

    switch(pid) {
        case -1:
            perror("fork failed");
            close(pipefd[0]);
            close(pipefd[1]);
            exit(EXIT_FAILURE);

        case 0:
            close(pipefd[0]); 
            const char *text_to_send = "cheCK tesTTTT";

            fprintf(stderr, "[Child] sending message: \"%s\"\n", text_to_send);

            int length = strlen(text_to_send);
            if (write(pipefd[1], text_to_send, length) != length) {
                perror("write failed");
                close(pipefd[1]);
                _exit(EXIT_FAILURE);
            }

            close(pipefd[1]);
            fprintf(stderr, "[Child] finished.\n");       
            _exit(EXIT_SUCCESS);
        default:
            close(pipefd[1]);

            char buffer[BUFF_SIZE];
            int bytes_read;

            while((bytes_read = read(pipefd[0], buffer, BUFF_SIZE)) > 0) {
                for (int i = 0; i < bytes_read; i++) {
                    buffer[i] = (char)toupper(buffer[i]);
                }

                if (write(STDOUT_FILENO, buffer, bytes_read) != bytes_read) {
                    perror("write failed");
                    close(pipefd[0]);
                    exit(EXIT_FAILURE);
                }
            }

            if (bytes_read == -1) {
                perror("read");
                exit(EXIT_FAILURE);
            }

            write(STDOUT_FILENO, "\n", 1);
            close(pipefd[0]);

            if (wait(NULL) == -1) {
                perror("wait");
                exit(EXIT_FAILURE);
            }

            printf("[Parent] Child process reaped. Exiting.\n");
            exit(EXIT_SUCCESS);
    }
}