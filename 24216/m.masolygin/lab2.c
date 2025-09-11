#include <stdio.h>
#include <time.h>
#include <stdlib.h>


int main()
{
    if (putenv("TZ=America/Los_Angeles")==-1){
        perror("Fail putenv");
        return 1;
    };  

    time_t now = time(NULL);
    
    printf("%s", ctime(&now));

    return 0;
}
