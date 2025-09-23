#include <sys/types.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

main()
{
    time_t now;
    struct tm *utc;
    struct tm pst;

    (void)time(&now);

    printf("Local time:\n");
    printf("%s", ctime(&now));

    utc = gmtime(&now); //UTC time
    pst = *utc;

    pst.tm_hour -= 8;
    pst.tm_isdst = 0;

    mktime(&pst);

    //PST time
    printf("\nPST time:\n");
    printf("%s", asctime(&pst));
    printf("%d/%d/%02d %d:%02d\n",
        pst.tm_mon + 1,
        pst.tm_mday,
       (pst.tm_year + 1900) % 100,
        pst.tm_hour,
        pst.tm_min);
}
