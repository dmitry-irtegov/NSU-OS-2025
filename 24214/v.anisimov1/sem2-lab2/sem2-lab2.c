#include <pthread.h> 
#include <stdio.h> 
#include <string.h> 

void PrintTenStrings(int thdNum) {
    for (int i = 1; i <= 10; i++) {
        printf("Thread %d: string %d\n", thdNum, i); 
    }
}

void * subroutine(void *arg) {
    PrintTenStrings(2); 
    return((void *)1); 
}

int main() {
    pthread_t cthd;
    int err = pthread_create(&cthd, NULL, subroutine, NULL);
    if (err != 0) {
        char buff[1024]; 
        strerror_r(err, buff, sizeof(buff)); 
        fprintf(stderr, "ERROR! Creation %s", buff); 
        return 0;  
    }
    err = pthread_join(cthd, NULL);
    if (err != 0) {
        char buff[1024]; 
        strerror_r(err, buff, sizeof(buff)); 
        fprintf(stderr, "ERROR! Join %s", buff); 
        return 0;  
    }
    PrintTenStrings(1); 
    return 0; 
}