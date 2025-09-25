#include <sys/types.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
extern char *tzname[];
int main()
{
    time_t now;
    struct tm *sp;
    putenv("TZ=America/Los_Angeles");
    tzset();
    time(&now);
    sp = localtime(&now);
    printf("%s", ctime(&now));
    int year = sp->tm_year+1900;
    printf("%d/%d/%02d %d:%02d\n",
        sp->tm_mon + 1, sp->tm_mday,
        year%100, sp->tm_hour,
        sp->tm_min);
    return 0;
}