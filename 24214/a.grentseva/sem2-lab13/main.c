#include <stdio.h>
#include <pthread.h>

pthread_mutex_t mutex;
pthread_cond_t cond;
int turn = 0; 

void* child_thread() {
    for (int i = 1; i <= 10; i++) {

        pthread_mutex_lock(&mutex);

        while (turn != 1) {
            if (pthread_cond_wait(&cond, &mutex) != 0) {
                fprintf(stderr, "child: cond_wait error\n");
                pthread_mutex_unlock(&mutex);
                return NULL;
            }
        }

        printf("child thread: line %d\n", i);

        turn = 0;
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&mutex);
    }

    return NULL;
}

int main() {
    pthread_t tid;

    if (pthread_mutex_init(&mutex, NULL) != 0 ||
        pthread_cond_init(&cond, NULL) != 0) {
        fprintf(stderr, "init error\n");
        return 1;
    }

    if (pthread_create(&tid, NULL, child_thread, NULL) != 0) {
        fprintf(stderr, "pthread_create error\n");
        return 1;
    }

    for (int i = 1; i <= 10; i++) {

        pthread_mutex_lock(&mutex);

        while (turn != 0) {
            if (pthread_cond_wait(&cond, &mutex) != 0) {
                fprintf(stderr, "parent: cond_wait error\n");
                pthread_mutex_unlock(&mutex);
                return 1;
            }
        }

        printf("parent thread: line %d\n", i);

        turn = 1;
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&mutex);
    }

    pthread_join(tid, NULL);

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);

    return 0;
}