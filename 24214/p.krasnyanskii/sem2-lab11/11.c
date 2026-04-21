#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

static pthread_mutex_t mutex_parent;
static pthread_mutex_t mutex_child;

void* child_thread(void* arg) {
    for (int i = 0; i < 10; i++) {
        pthread_mutex_lock(&mutex_child);

        printf("Child thread: %d\n", i + 1);

        pthread_mutex_unlock(&mutex_parent);
    }
    return NULL;
}

int main(void) {
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL);
    pthread_mutex_init(&mutex_parent, &attr);
    pthread_mutex_init(&mutex_child,  &attr);
    pthread_mutexattr_destroy(&attr);

    pthread_mutex_lock(&mutex_child);

    pthread_t tid;
    if (pthread_create(&tid, NULL, child_thread, NULL) != 0) {
        perror("pthread_create");
        return 1;
    }

    for (int i = 0; i < 10; i++) {
        pthread_mutex_lock(&mutex_parent);

        printf("Parent thread: %d\n", i + 1);

        pthread_mutex_unlock(&mutex_child);
    }

    pthread_join(tid, NULL);

    pthread_mutex_destroy(&mutex_parent);
    pthread_mutex_destroy(&mutex_child);

    return 0;
}

