#include <stdio.h>
#include <time.h>
#include <stdlib.h>

int main(void) {
    if(putenv("TZ=America/Los_Angeles")) {
        perror("Couldn't set TZ");
    }
    time_t cur_time = time(NULL);
    printf("%s\n", ctime(&cur_time));
    return 0;
}
