#include <sys/types.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

int main() {
    time_t now;
    if (setenv("TZ", "America/Los_Angeles", 1) != 0) {
        perror("setenv failed");
        return -1;
    }
    tzset();
    if (time(&now) == (time_t)-1) {
        perror("time failed");
        return -1;
    }
    char *time_str = ctime(&now);
    if (time_str == NULL) {
        perror("ctime failed");
        return -1;
    }

    printf("UTC time: %s", time_str);
    return 0;
}
