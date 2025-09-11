#include <sys/types.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

int main()
{
    time_t now;
    
    int f = putenv("TZ=US/Los_Angeles");
    if (f != 0) {
        perror("putenv failed");
        exit(1);
    }
    
    time(&now);

    printf("%s", ctime(&now));
    return;
    
}