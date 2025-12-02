#include <sys/types.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

extern char *tzname[];

int main()
{
    setenv("TZ", "America/Los_Angeles", 1);
    tzset();

    time_t now;
    struct tm *sp;
    (void)time(&now);
    
    printf("%s", ctime(&now));
    
    sp = localtime(&now);
    
    const char* timezone_name;
    if (sp->tm_isdst >= 0 && sp->tm_isdst <= 1)
    {
        timezone_name = tzname[sp->tm_isdst];
    }
    
    else
    {
        timezone_name = "???";
    }
    
    printf("%d/%d/%04d %d:%02d %s\n", sp->tm_mon + 1, sp->tm_mday, 1900 + sp->tm_year, sp->tm_hour, sp->tm_min, timezone_name);
    exit(0);
}