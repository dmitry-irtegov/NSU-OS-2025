#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/wait.h>


#define BUFFER_SIZE 12

int main() {
    int fd[2];

    char text[] = "HellO! This text GOes via PIPE.\n";
    if (pipe(fd) == -1) {
        perror("pipe error");
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();

    switch (pid) {
        case -1:
            perror("fork error");
            exit(EXIT_FAILURE);

        case 0: {
            if (close(fd[0]) == -1) {
                perror("close reading end error");
                exit(EXIT_FAILURE);
            }

            ssize_t written = write(fd[1], text, strlen(text));
            if (written == -1) {
                perror("write to pipe error");
                exit(EXIT_FAILURE);
            }

            if (close(fd[1]) == -1) {
                perror("close writing end error");
                exit(EXIT_FAILURE);
            }
            
            exit(EXIT_SUCCESS);
        }

        default: {
            char buf[BUFFER_SIZE];
            ssize_t bytes_read;


            if (close(fd[1]) == -1) {
                perror("close writing end error");
                exit(EXIT_FAILURE);
            }

            while ((bytes_read = read(fd[0], buf, BUFFER_SIZE)) > 0) {

                for (int i = 0; i < bytes_read; i++) {
                    buf[i] = toupper((unsigned char)buf[i]);
                }


                if (write(STDOUT_FILENO, buf, bytes_read) == -1) {
                    perror("write to stdout error");
                    exit(EXIT_FAILURE);
                }
            }


            if (bytes_read == -1) {
                perror("read from pipe error");
                exit(EXIT_FAILURE);
            }

            if (close(fd[0]) == -1) {
                perror("close reading end error");
                exit(EXIT_FAILURE);
            }


            wait(NULL);
            exit(EXIT_SUCCESS);
        }
    }
}