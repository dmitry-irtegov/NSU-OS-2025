#include <sys/types.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

int main() {
    if (putenv("TZ=PST8PDT") != 0) {
        perror("Failed putenv");
        return 1;
    }

    time_t now = time(0);
    if (now == (time_t)-1) {
        perror("Failed time");
        return 1;
    }

    char *res = ctime(&now);
    if (res == NULL) {
        perror("Failed ctime");
        return 1;
    }

    printf("%s", res);

    return 0;
}