#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MSG_SIZE 256

int main() {
    char message[MSG_SIZE] = "abvGDEe zZiYkl mnOPQ RST";

    FILE *pipe = popen("tr 'a-z' 'A-Z'", "w");
    if (pipe == NULL) {
        perror("popen");
        exit(EXIT_FAILURE);
    }

    if (fputs(message, pipe) == EOF) {
        perror("fputs");
        exit(EXIT_FAILURE);
    }

    pclose(pipe);

    return 0;
}
