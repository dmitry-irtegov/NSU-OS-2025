#include <stdio.h>
#include <time.h>
#include <stdlib.h>


int main(){

    if (putenv("TZ=America/Los_Angeles") == -1) {
        perror("putenv error");
        return 1;
    } 

    time_t now = time(NULL);
    
    char* result = ctime(&now);
    if (result == NULL){
        perror("ctime error");
        return 1;
    }
    printf("%s", result);

    return 0;
}