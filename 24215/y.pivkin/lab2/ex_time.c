#define _POSIX_C_SOURCE 200112L
#include <sys/types.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
extern char *tzname[];

int main(){
    setenv("TZ", "US/Pacific", 1);
    tzset();

    time_t now;
    struct tm *pst;

    (void)time(&now);

    printf("%s", ctime(&now));

    pst = localtime(&now);
    printf("%d/%d/%02d %d:%02d %s\n",
        pst->tm_mon + 1,
        pst->tm_mday,
       (pst->tm_year + 1900) % 100,
        pst->tm_hour,
        pst->tm_min,
        tzname[pst->tm_isdst]);

    return 0;
}
