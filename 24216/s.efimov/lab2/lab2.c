#include <stdio.h>
#include <time.h>
#include <stdlib.h>

int main(){
    if (putenv("TZ=America/Los_Angeles") != 0){
        perror("Falling to get current time.\n");
        exit(EXIT_FAILURE);
    }

    time_t CaliforniaTime;
    
    if (time(&CaliforniaTime) == (time_t)(-1)){
        perror("You can't change the TZ variable to find out the time in California.\n");
        exit(EXIT_FAILURE);
    }

    char* now = ctime(&CaliforniaTime);

    if (now == NULL){
        perror("Fail ctime.\n");
        exit(EXIT_FAILURE);
    }
    printf("Current Date and Time: %s\n", now);
    
    exit(EXIT_SUCCESS);
}

