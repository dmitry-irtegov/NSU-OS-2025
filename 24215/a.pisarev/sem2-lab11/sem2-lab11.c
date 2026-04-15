#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sched.h>

#define PARENT_THREAD 0
#define CHILD_THREAD 1

pthread_mutex_t lock;
int turn = PARENT_THREAD;

void *printTenRows(void *arg) {
    int id = *(int *)arg;
    
    for (int i = 0; i < 10; i++) {
        while (1) {
            pthread_mutex_lock(&lock);
            if (turn == id) {
                printf("Current number is:%d (%d)\n", i, id);
                turn = (id == PARENT_THREAD) ? CHILD_THREAD : PARENT_THREAD;
                pthread_mutex_unlock(&lock);
                break;
            }
            pthread_mutex_unlock(&lock);
            sched_yield();
        }
    }
    return NULL;
}

int main() {
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
    pthread_mutex_init(&lock, &attr);
    pthread_mutexattr_destroy(&attr);
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
    return 0;
}
