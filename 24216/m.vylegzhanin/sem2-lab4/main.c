#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

void *child_thread() {
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

    int counter = 0;
    while (1) {
        printf("Дочерняя нить работает... итерация %d\n", ++counter);
        fflush(stdout);
        sleep(1);
    }

    return NULL;
}

int main(void) {
    pthread_t tid;

    if (pthread_create(&tid, NULL, child_thread, NULL) != 0) {
        perror("pthread_create");
        exit(EXIT_FAILURE);
    }

    printf("Дочерняя нить создана\n");

    sleep(2);

    printf("Завершение дочерней нити\n");

    if (pthread_cancel(tid) != 0) {
        perror("pthread_cancel");
        exit(EXIT_FAILURE);
    }

    if (pthread_join(tid, NULL) != 0) {
        perror("pthread_join");
        exit(EXIT_FAILURE);
    }

    printf("дочерняя нить завершена\n");

    return 0;
}
