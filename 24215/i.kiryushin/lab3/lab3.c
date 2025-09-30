#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

int main(){
    FILE* f;
    uid_t real_uid, eff_uid;

    real_uid = getuid();
    printf("Real UID: %u\n", real_uid);

    eff_uid = geteuid();
    printf("Effective UID: %u\n", eff_uid);

    f = fopen("file.txt", "r");
    
    if (f == NULL){
        perror("Error with opening file");
    }
    else{
        fclose(f);
    }
    
    if (setuid(real_uid) == -1){
        perror("Error setting UID");
    }

    real_uid = getuid();
    printf("Real UID: %u\n",real_uid);

    eff_uid = geteuid();
    printf("Effective UID: %u\n",eff_uid);

    f = fopen("file.txt", "r");

    if (f == NULL){
        perror("Error opening file");
    }
    else{
        fclose(f);
    }

    return 0;
}