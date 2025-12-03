#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MSG_SIZE 256

int main() {
    char package[MSG_SIZE] = "Hello Gophers\n";

    FILE *pp = popen("tr 'a-z' 'A-Z'", "w");
    if (pp == NULL) {
        perror("popen error");
        exit(EXIT_FAILURE);
    }

    if (fputs(package, pp) == EOF) {
        perror("fputs error");
        exit(EXIT_FAILURE);
    }

    pclose(pp);

    exit(EXIT_SUCCESS);
}
