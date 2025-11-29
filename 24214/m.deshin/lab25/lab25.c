#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <sys/wait.h>

#define TEXT_SIZE 256

int main() {
    char text[] = "I need to pass lab25!";
    int p[2];
    pid_t pid;

    if (pipe(p) == -1) {
        perror("pipe failed");
        return 1;
    }

    switch (pid = fork()) {
        case -1:
            perror("fork failed");
            return 1;
        case 0:
            close(p[0]);

            ssize_t bytes_written = write(p[1], text, strlen(text));
            if (bytes_written == -1) {
                perror("write failed");
                close(p[1]);
                return 1;
            }

            close(p[1]);
            return 0;
        default:
            close(p[1]);

            char received_text[TEXT_SIZE];
            ssize_t bytes_read = read(p[0], received_text, TEXT_SIZE - 1);
            if (bytes_read == -1) {
                perror("read failed");
                close(p[0]);
                return 1;
            }
            received_text[bytes_read] = '\0';

            close(p[0]);

            for (ssize_t i = 0; i < bytes_read; i++) {
                received_text[i] = toupper((unsigned char) received_text[i]);
            }

            printf("Modified text: %s\n", received_text);

            if (waitpid(pid, NULL, 0) == -1) {
                perror("waitpid failed");
                return 1;
            }

            return 0;
    }

    return 0;
}
