#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

void printID() {
    uid_t real = getuid();
    uid_t effective = geteuid();
    printf("Real user ID: %d\n", real);
    printf("Effective user ID: %d\n", effective);
}

void openFile(char* filename) {
    FILE* f;
    if (!(f = fopen(filename, "r"))) {
        perror("Couldn't open file");
        return;
    }
    if (fclose(f)) {
        perror("Couldn't close file");
    }
}

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "Enter file name\n");
        return 1;
    }
    printf("Before setuid:\n");
    printID();
    openFile(argv[1]);

    if(setuid(getuid()) == -1) {
        perror("setuid");
        return 1;
    }
    printf("After setuid:\n");
    printID();
    openFile(argv[1]);
    return 0;
}