#include <sys/types.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>


int main(){
    int status = putenv("TZ=America/Los_Angeles");
    
    time_t CaliforniaTime;
    struct tm *now;

    if (status != 0){
        perror("You can't change the TZ variable to find out the time in California.\n");
        exit(1);
    }

    time(&CaliforniaTime);
    now = localtime(&CaliforniaTime);

    printf("Date: %d.%d.%d\n", now->tm_mday, now->tm_mon + 1, now->tm_year + 1900);
    printf("Time: %d:%d:%d\n",now->tm_hour,now->tm_min,now->tm_sec);
    exit(0);
}

