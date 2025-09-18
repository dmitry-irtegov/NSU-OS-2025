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
    char *t = ctime(&now);
    if (t == NULL) {
        perror("ctime fail");
        exit(EXIT_FAILURE);
    }
    printf("%s", t);

    return 0;
}
