#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

void* thread_func(void* arg) {
    for (int i = 0; i < 10; i++) {
        printf("Child thread: line %d\n", i + 1);
        usleep(100000);
    }
    return NULL;
}

int main() {
    pthread_t tid;

    pthread_create(&tid, NULL, thread_func, NULL);

    for (int i = 0; i < 10; i++) {
        printf("Parent thread: line %d\n", i + 1);
        usleep(100000);
    }

    pthread_join(tid, NULL);

    return 0;
}
