#include <stdio.h>
#include <time.h>
#include <stdlib.h>
int main(){
    if (setenv("TZ", "PST8PDT", 1) == -1) {
        perror("Error setenv:");
        exit(1);
    }
    time_t now;
    if(time(&now) == -1){
        perror("Error time:");
        exit(1);
    }
    char* res = ctime(&now);
    if(res == NULL){
        perror("Error ctime");
        exit(1);
    }
    printf("Time in California: %s", res);
    exit(0);
}
