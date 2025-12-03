#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main() {
    putenv("TZ=PST8PDT");
    tzset();
    
    time_t rawtime;
    
    time(&rawtime);
    
    char* time_str = ctime(&rawtime);
    
    printf("Текущее время в Калифорнии: %s", time_str);
    
    return 0;
}