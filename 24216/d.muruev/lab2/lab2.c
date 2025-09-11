#include <sys/types.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

int main()
{
    time_t now;

    if (setenv("TZ", "America/Los_Angeles", 1) == -1) {
        perror("Can not change the local variable TZ");
        exit(1);
    }

    tzset();

    time(&now);

    printf("%s", ctime(&now));

    exit(0);

}