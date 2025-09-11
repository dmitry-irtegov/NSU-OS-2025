#include <stdio.h>
#include <time.h>
#include <stdlib.h>

int main(void){
    time_t now;
    
    if (time(&now) == (time_t)-1) {
        perror("Failed to get current time");
        exit(EXIT_FAILURE);
    }
    
    if (setenv("TZ", "America/Los_Angeles", 1) == -1) {
        perror("Failed to set environment variable");
        exit(EXIT_FAILURE);
    }
    
    tzset();

    char *time_str = ctime(&now);
    if (time_str == NULL) {
        fprintf(stderr, "Failed to convert time to string\n");
        exit(EXIT_FAILURE);
    }
    
    printf("%s", time_str);
    
    return EXIT_SUCCESS;
}