#include <stdio.h>
#include <time.h>
#include <stdlib.h>


int main(){
    if (putenv("TZ=America/Los_Angeles") != 0) {
        perror("Fail putenv");
        return 1;
    } 

    time_t now = time(NULL);
    
    char* result = ctime(&now);
    if (result == NULL){
        perror("Fail ctime");
        return 1;
    }
    printf("%s", result);

    return 0;
}
