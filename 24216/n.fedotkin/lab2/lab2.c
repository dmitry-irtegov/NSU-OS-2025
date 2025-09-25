#include <sys/types.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

int main() {
    time_t now;
    struct tm *sp;

    int result = putenv("TZ=America/Los_Angeles");
    if (result != 0) {
        perror("putenv failed");
        exit(EXIT_FAILURE);
    }

    time_t res = time(&now);
    if (res == (time_t)-1) {
        perror("time failed");
        exit(EXIT_FAILURE);
    }

    char* time = ctime(&now);
    if (time == NULL) {
        perror("ctime failed");
        exit(EXIT_FAILURE);
    }

    printf("%s", time);

    exit(0);
}
