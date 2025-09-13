#include <stdio.h>
#include <time.h>
#include <stdlib.h>


int main(){

    if (putenv("TZ=America/Los_Angeles") == -1) {
        perror("putenv error");
        exit(1);
    } 

    time_t now = time(NULL);
    
    if (now == (time_t)(-1)){
        perror("time error");
        exit(1);
    }

    char* result = ctime(&now);
    if (result == NULL){
        perror("ctime error");
        exit(1);
    }

    printf("%s", result);

    exit(0);
}