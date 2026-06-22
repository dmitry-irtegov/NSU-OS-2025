#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

void* child_func() {
    printf("Дочерняя нить запущена.\n");
    int i = 0;
    while(1) {
        printf("Дочерняя нить работает %d сек\n", ++i);
        sleep(1); 
    }
    return NULL;
}

int main() {
    pthread_t thread;
    pthread_create(&thread, NULL, child_func, NULL);
    printf("Родитель ждет 2 секунды\n");
    sleep(2);
    pthread_cancel(thread);
    printf("Родитель отменяет дочернюю нить\n");
    printf("дочерняя нить была отменена");
    return 0;
}
