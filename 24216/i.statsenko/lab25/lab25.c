#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/wait.h>

#define MAX_PACKAGE 1024

int main() {
    char package[MAX_PACKAGE] = "Hello, Gophers";
    int pp[2];

    if (pipe(pp) == -1) {
        perror("error in pipe");
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();

    switch (pid) {
        case -1:
            perror("error in fork");
            exit(EXIT_FAILURE);

        case 0:
            close(pp[1]);

            char newPackage[MAX_PACKAGE];
            ssize_t countBytesRead = read(pp[0], newPackage, MAX_PACKAGE);
            if (countBytesRead == -1) {
                perror("error in read");
                exit(EXIT_FAILURE);
            }

            for (ssize_t i = 0; i < countBytesRead; i++) {
                newPackage[i] = toupper(newPackage[i]);
            }

            printf("toUpper = %s\n", newPackage);

            close(pp[0]);
            exit(EXIT_SUCCESS);

        default:
            close(pp[0]);  

            ssize_t countBytesWrite = write(pp[1], package, strlen(package) + 1);
            if (countBytesWrite == -1) {
                perror("error in write");
                exit(EXIT_FAILURE);
            }

            close(pp[1]);

            if (waitpid(pid, NULL, 0) == -1) {
                perror("error in waitpid");
                exit(EXIT_FAILURE);
            }
            exit(EXIT_SUCCESS);
    }
}
