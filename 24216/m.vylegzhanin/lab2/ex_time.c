#include <sys/types.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

main()
{
    time_t now;
    time(&now);
    if (setenv("TZ", "America/Los_Angeles", 1) == -1){
        perror("failed to set environment variable");
        exit(0);
    }
    tzset(); 
    printf("%s", ctime(&now));
    exit(0);
}