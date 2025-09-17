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
        return 1;
    }
    
    time(&now);
    
    char * rs = ctime(&now);
    
    if (rs == NULL) {
        perror("ctime failed");
        return 1;
    }

    printf("%s", rs);
    
    return 0;

}