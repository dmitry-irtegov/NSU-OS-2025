#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

void printId(){
    printf("Real ID: %d\n", getuid());
    printf("Effective ID: %d\n", geteuid());
}

void checkToOpenFile(FILE* file) {
    if (file == NULL) {
        perror("Can't open the file.\n");
    } else {
        printf("The file was opened.\n");
        fclose(file);
    }
}

int main(){
    const char* name = "secret.txt";

    printId();

    FILE *file = fopen(name, "rw");

    checkToOpenFile(file);

    if (setuid(getuid()) == -1) {
        perror("FAIL: setuid()");
        exit(EXIT_FAILURE);
    }

    printId();

    file = fopen(name, "rw");
    checkToOpenFile(file);

    return 0;
}
