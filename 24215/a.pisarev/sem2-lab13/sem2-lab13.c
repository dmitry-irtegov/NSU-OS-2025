#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define PARENT_THREAD 0
#define CHILD_THREAD 1
pthread_mutex_t lock;
pthread_cond_t  cond;
int turn = PARENT_THREAD;

void *printTenRows(void *arg) {
    int id = *(int*)arg;
    for (int i = 0; i < 10; i++) {
        pthread_mutex_lock(&lock);
        while (turn != id) {
            pthread_cond_wait(&cond, &lock);
        }
        printf("Current number is:%d (%d)\n", i, id);
        turn = (id == PARENT_THREAD) ? CHILD_THREAD : PARENT_THREAD;
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&lock);
    }
    return NULL;
}

int main() {
    pthread_mutex_init(&lock, NULL);
    pthread_cond_init(&cond, NULL);
    pthread_t thread;
    int thread_num = CHILD_THREAD;
    if (pthread_create(&thread, NULL, printTenRows, &thread_num) != 0) {
        perror("Creating thread");
        exit(1);
    }
    int main_thread_num = PARENT_THREAD;
    printTenRows(&main_thread_num);
    pthread_join(thread, NULL);
    pthread_mutex_destroy(&lock);
    pthread_cond_destroy(&cond);

    return 0;
}
