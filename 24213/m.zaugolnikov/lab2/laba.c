#include <sys/types.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

extern char *tzname[];

int main()  {
    time_t now;
    struct tm *sp;

    int status = putenv("TZ=America/Los_Angeles");

    if (status != 0){
        perror("Problem with changing local variable TZ");
        exit(1);
    }

    time(&now);

    printf("%s", ctime(&now));

    sp = localtime(&now);

    printf("%d/%d/%02d %d:%02d %s %d\n",
        sp->tm_mon + 1, sp->tm_mday,
        sp->tm_year, sp->tm_hour,
        sp->tm_min, tzname[sp->tm_isdst], sp->tm_isdst);

    exit(0);
}

