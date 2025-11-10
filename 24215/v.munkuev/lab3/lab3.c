#include <stdio.h>
#include <unistd.h>
#include <errno.h>


void open_file(char* filename){
    FILE* f = fopen(filename, "r+");
    if (f){
        printf("file is succesfully opened!\n");
        fclose(f);
    }
    else{
        printf("can't open file!\n");
    }
}

int main(){
    uid_t ruid, euid;

    //До setuid()
    ruid = getuid();
    euid = geteuid();
    printf("Before setuid:\n    RUID = %ld\n    EUID = %ld\n", (long)ruid, (long)euid);
    open_file("file");

    //Ставим ruid и euid одинаковыми
    if (setuid(ruid) == -1) {
        perror("setuid(getuid())");
    }

    //После setuid()
    ruid = getuid();
    euid = geteuid();
    printf("\nAfter setuid:\n    RUID = %ld\n    EUID = %ld\n", (long)ruid, (long)euid);
    open_file("file");

    return 0;
}