#include <stdio.h>
#include <time.h>
#include <stdlib.h>

int main(void) {
    if(putenv("TZ=America/Los_Angeles")) {
        perror("Couldn't set TZ");
        return 1;
    }
    time_t cur_time = time(NULL);
    if (cur_time == (time_t)-1) {
        perror("Something went wrong with time");
        return 1;
    }
    char* time_string = ctime(&cur_time);
    if (!time_string) {
        perror("Something went wrong with ctime");
        return 1;
    }
    printf("%s\n", time_string);
    return 0;
}
