#include <stdio.h>
#include <sys/types.h> 
#include <unistd.h>

void fileOpen(const char* filename);

int main(){
    const char* filename = "file.txt";

    uid_t real = getuid();
    uid_t effective = geteuid();

    printf("-------------------------\n");
    printf("Without setuid:\n");
    printf("Real user ID: %d\n", real);
    printf("Effective user ID: %d\n", effective);
    printf("-------------------------\n");

    fileOpen(filename);

    if (setuid(getuid()) == -1){
        perror("Fail setuid");
        return 1;
    }

    real = getuid();
    effective = geteuid();

    printf("-------------------------\n");
    printf("With setuid:\n");
    printf("Real user ID: %d\n", real);
    printf("Effective user ID: %d\n", effective);
    printf("-------------------------\n");

    fileOpen(filename);
    return 0;
}

void fileOpen(const char* filename){
    FILE* file;
    if (!(file = fopen(filename, "r"))){
        perror("Coudn't open file");
        return;
    }
    if (fclose(file)) {
        perror("Couldn't close file");
        return;
    }

    printf("-------Open success-------\n");
}
