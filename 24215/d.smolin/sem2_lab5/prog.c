#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

volatile int x = 0;

void cleanup(void* arg) {
    //x = 1;
    printf("%d\n", x);
}

void* child_func() {
    printf("Дочерняя нить запущена.\n");
    pthread_cleanup_push(cleanup, NULL);
    x = 1;
    int i = 0;
    while(1) {
        printf("Дочерняя нить работает %d сек\n", ++i);
        sleep(1); 
    }
    pthread_cleanup_pop(0);
    return NULL;
}

int main() {
    pthread_t thread;
    pthread_create(&thread, NULL, child_func, NULL);
    printf("Родитель ждет 2 секунды\n");
//    sleep(2);
    pthread_cancel(thread);
    while(x == 0) {
    }
    printf("Родитель отменяет дочернюю нить\n");
    printf("дочерняя нить была отменена");
    return 0;
}
