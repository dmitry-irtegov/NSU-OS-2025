#include <stdio.h>
#include <time.h>
#include <stdlib.h>


int main(){
    if (putenv("TZ=America/Los_Angeles") == -1) {
        perror("Fail putenv");
        return 1;
    } 

    time_t now = time(NULL);
    
    char* res = ctime(&now);

    if (res == NULL){
        perror("Fail ctime");
        return 1;
    }
    
    printf("%s", res);

    return 0;
}