#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

void* child_task(void* arg) {
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

    while (1) {
        printf("-> Дочерняя нить: я работаю...\n");
        sleep(1); 
    }
    return NULL;
}

int main() {
    pthread_t thread_id;
    void* result;
    int s;

    printf("Родитель: Создание нити...\n");
    s = pthread_create(&thread_id, NULL, child_task, NULL);
    if (s != 0) {
        errno = s; 
        perror("pthread_create error");
        return 1;
    }
    sleep(2);

    printf("Родитель: Отправка сигнала отмены...\n");
    s = pthread_cancel(thread_id);
    if (s != 0) {
        errno = s;
        perror("pthread_cancel error");
        return 1;
    }

    s = pthread_join(thread_id, &result);
    if (s != 0) {
        errno = s;
        perror("pthread_join error");
        return 1;
    }

    if (result == PTHREAD_CANCELED) {
        printf("Родитель: Нить была корректно отменена.\n");
    } else {
        printf("Родитель: Странно, нить завершилась сама (чего не должно быть).\n");
    }

    return 0;
}
