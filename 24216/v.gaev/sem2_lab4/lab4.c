#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

void* child_task(void* arg) {
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
    while (1) {
        fprintf(stderr, "Дочерняя нить: работает\n");
        sleep(1); 
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    const char *name_prog = argv[0];
    
    pthread_t thread_id;
    void* result;
    int s;

    fprintf(stderr, "Родительская нить: Создание нити\n");
    s = pthread_create(&thread_id, NULL, child_task, NULL);
    if (s != 0) {
        const char *action = "creating thread";
        char buf[256];
        strerror_r(s, buf, sizeof buf);
        fprintf(stderr, "%s: %s: %s\n", name_prog, action, buf);
        exit(1);
    }
    
    sleep(2);

    fprintf(stderr, "Родительская нить: Отправка сигнала отмены\n");
    s = pthread_cancel(thread_id);
    if (s != 0) {
        const char *action = "canceling thread";
        char buf[256];
        strerror_r(s, buf, sizeof buf);
        fprintf(stderr, "%s: %s: %s\n", name_prog, action, buf);
        exit(1);
    }

    s = pthread_join(thread_id, &result);
    if (s != 0) {
        const char *action = "joining thread";
        char buf[256];
        strerror_r(s, buf, sizeof buf);
        fprintf(stderr, "%s: %s: %s\n", name_prog, action, buf);
        exit(1);
    }

    if (result == PTHREAD_CANCELED) {
        fprintf(stderr, "Родительская нить: Нить была корректно отменена.\n");
    } else {
        fprintf(stderr, "Родительская нить: Нить завершилась сама.\n");
    }

    return 0;
}
