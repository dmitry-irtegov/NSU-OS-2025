#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <time.h>

int main() {
    time_t now;
    (void)time(&now);

    printf("Time Los Angeles: %s", ctime(&now));
}