#include <sys/types.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

char buffer[20];

int main(){
    int status = putenv("TZ=America/Los_Angeles");
    
    if (status != 0){
        perror("You can't change the TZ variable to find out the time in California.\n");
        exit(EXIT_FAILURE);
    }

    time_t CaliforniaTime;
    struct tm *now;

    time(&CaliforniaTime);
    now = localtime(&CaliforniaTime);
    strftime(buffer, sizeof(buffer), "%d.%m.%Y %H:%M:%S", now);

    printf("Current Date and Time: %s\n", buffer);
    
    exit(EXIT_SUCCESS);
}

