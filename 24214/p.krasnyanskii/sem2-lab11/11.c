#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  cond = PTHREAD_COND_INITIALIZER;
int turn = 0;

void* child_thread(void* arg) {
    for (int i = 0; i < 10; i++) {
        pthread_mutex_lock(&mtx);
        while (turn != 1)
            pthread_cond_wait(&cond, &mtx);
        printf("Child thread: %d\n", i + 1);
        turn = 0;
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&mtx);
        usleep(100000);
    }
    return NULL;
}

int main(void) {
    pthread_t tid;
    pthread_create(&tid, NULL, child_thread, NULL);

    for (int i = 0; i < 10; i++) {
        pthread_mutex_lock(&mtx);
        while (turn != 0)
            pthread_cond_wait(&cond, &mtx);
        printf("Parent thread: %d\n", i + 1);
        turn = 1;
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&mtx);
        usleep(100000);
    }

    pthread_join(tid, NULL);
    return 0;
}

