#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main() {

    if (setenv("TZ", "America/Los_Angeles", 1) != 0) {
        perror("setenv fail");
        exit(EXIT_FAILURE);
    }
    tzset();

    time_t now = time(NULL);
    printf("%s", ctime(&now));

    return 0;
}