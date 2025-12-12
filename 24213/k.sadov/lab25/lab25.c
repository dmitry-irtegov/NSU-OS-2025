#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>

#define BUFF_SIZE 20

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
        printf("Child process started. PID: %d\n", getpid());
        close(write_pipefd);

        char b[BUFF_SIZE];
        ssize_t read_bytes;

        while ((read_bytes = read(read_pipefd, b, sizeof(b))) > 0) {
            for (int i = 0; i < read_bytes; i++) {
                b[i] = toupper(b[i]);
            }

            if (write(STDOUT_FILENO, b, read_bytes) == -1) {
                perror("write failed");
                exit(EXIT_FAILURE);
            }
        }

        close(read_pipefd);
        exit(EXIT_SUCCESS);
    }

    close(read_pipefd);
    printf("Parent process sending data. PID: %d\n", getpid());
    const char *msg = "soMe TeXt FoR TesT\n";
    printf("Original message: %s", msg);
    size_t msg_length = strlen(msg);

    while (msg_length > 0) {
        ssize_t sent = write(pipefd[1], msg, msg_length);
        if (sent == -1) {
            perror("write failed");
            exit(EXIT_FAILURE);
        }
        msg += sent;
        msg_length -= sent;
    }

    close(write_pipefd);
    if (wait(NULL) == -1) {
        perror("wait failed");
        exit(EXIT_FAILURE);
    }
    exit(EXIT_SUCCESS);
}
