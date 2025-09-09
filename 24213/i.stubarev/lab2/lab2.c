#include <sys/types.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
extern char *tzname[];
int main()
{
    time_t now;
    struct tm *sp;

    //california is located in the UTC-8 time zone.
    //The TZ stores the number that is obtained when UTS = PST + TZ.
    //PDT is the Pacific Daylight Time (летнее время)
    setenv("TZ", "PST+8PDT", 1);

    //to make changes to the environment variable take effect immediately
    //tzset(); 

    //time() - writes to now the value in seconds of how much time has passed since January 1, 1970, 00:00:00 UTC
    (void)time(&now);

    /**ctime() - Converts the value to local time (as localtime) 
    and formats it into a string like Wed Jun 15 11:38:07 1988\n\0 (as asctime).*/
    //do tzset() inside
    printf("%s", ctime(&now));

    //localtime() - converts time in local time, then converts the arithmetic representation to the traditional representation defined by struct tm.
    sp = localtime(&now);
    printf("%d/%d/%02d %d:%02d %s\n",
           sp->tm_mon + 1, sp->tm_mday,
           sp->tm_year%100, sp->tm_hour,
           sp->tm_min, tzname[sp->tm_isdst]);
    exit(0);
}
