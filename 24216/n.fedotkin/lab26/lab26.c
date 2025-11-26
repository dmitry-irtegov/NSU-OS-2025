#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_SIZE 23

int main() {
    char message[BUFFER_SIZE] = "I very love os course!\n";

    FILE *pipe = popen("tr 'a-z' 'A-Z'", "w");
    if (pipe == NULL) {
        perror("Error: popen");
        exit(EXIT_FAILURE);
    }

    if (fputs(message, pipe) == EOF) {
        perror("Error: fputs");
        exit(EXIT_FAILURE);
    }

    pclose(pipe);

    exit(EXIT_SUCCESS);
}