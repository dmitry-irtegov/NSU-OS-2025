#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

int try_open_file(const char* filename){
FILE* f = fopen(filename, "r");
uid_t r = getuid();
uid_t e = geteuid();
printf("Real_uid: %d\n", r);
printf("effective_uid: %d\n", e);
    if (f){
        printf("fopen successful\n");
        fclose(f);
        return 0;
    }
    else{
        perror("fopen error");
        return -1;
    }
}

int main(){
    char* filename = "perm.txt";
    uid_t e = geteuid();
    try_open_file(filename);
    if (setuid(e) == -1){
        perror("setuid error");
        exit(EXIT_FAILURE);
    }
    try_open_file(filename);
    exit(0);
}