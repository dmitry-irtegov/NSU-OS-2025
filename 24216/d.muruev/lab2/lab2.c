#include <sys/types.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

int main() {
    time_t now;

    int env = setenv("TZ", "America/Los_Angeles", 1);
    if (env != 0) {
        perror("Can not change the local variable TZ");
        exit(EXIT_FAILURE);
    }

    if (time(&now) == (time_t)-1) {
        perror("Can not get the current time");
        exit(EXIT_FAILURE);
    }

    char *time_str = ctime(&now);
    if (time_str == NULL) {
        perror("Can not convert the current time");
        exit(EXIT_FAILURE);
    }

    printf("%s", time_str);

    exit(0);

    
}