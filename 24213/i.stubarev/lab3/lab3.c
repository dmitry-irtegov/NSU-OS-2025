#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

void printIDs(uid_t ruid, uid_t euid){
    printf("Real user ID: %d\n", ruid);
    printf("Effective user ID: %d\n", euid);
}

void openFile(char* name){
    FILE *file = fopen(name, "rw");
    if (file == NULL){
        perror("Couldn't open the file");
        exit(1);
    }
    printf("The file was opened\n");
    fclose(file);
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "You need to enter the file name");
        exit(1);
    }
    uid_t r_uid = getuid();
    uid_t e_uid = geteuid();

    printf("Before setuid():\n");
    printIDs(r_uid, e_uid);
    openFile(argv[1]);

    if(setuid(r_uid) == -1){ //if we want to be able to return the rights from e_uid, we can run seteuid
        perror("setuid failed");
        exit(1);
    }
    r_uid = getuid();
    e_uid = geteuid();

    printf("\nAfter setuid():\n");
    printIDs(r_uid, e_uid);
    openFile(argv[1]);

    exit(0);
}
