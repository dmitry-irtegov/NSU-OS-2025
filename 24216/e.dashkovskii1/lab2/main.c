#include <sys/types.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

extern char *tzname[];

int main(void) {
    time_t now;
    struct tm *sp;

    putenv("TZ=PST8PDT");
    tzset();

    (void) time(&now);

    sp = localtime(&now);

    printf("%02d/%02d/%04d %02d:%02d:%02d (%s)\n",
        sp->tm_mon + 1, sp->tm_mday,
        sp->tm_year + 1900, sp->tm_hour,
        sp->tm_min, sp->tm_sec,
        tzname[sp->tm_isdst]);

    return 0;
}