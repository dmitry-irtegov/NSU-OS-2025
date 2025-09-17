#include <stdio.h>
#include <time.h>
#include <stdlib.h>

int main() {
    time_t now = time(NULL);

    int operation_result = setenv("TZ", "America/Los_Angeles", 1);
    if (operation_result != 0) {
        perror("setenv   failed");
        exit(1);
    }

    char* answer = ctime(&now);
    if (answer == NULL) {
        perror(" getting    time  failed");
        exit(1);
    }

    printf(" Current time = %s", answer);

    exit(0);
}