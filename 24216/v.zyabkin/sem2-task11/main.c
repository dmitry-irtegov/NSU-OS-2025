#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct SafeSync {
    pthread_mutex_t mutex_parent;
    pthread_mutex_t mutex_child;
} SyncContext;

typedef struct ThreadArgs {
    SyncContext* sync;
    char* text;
    int is_parent; // 1 - parent's turn, 0 - child's turn
} ThreadArgs;

void thread_task(ThreadArgs* args) {
    for (int i = 1; i <= 100; i++) {
        if (args->is_parent) {
            pthread_mutex_lock(&args->sync->mutex_parent);
            fprintf(stderr, "%s%d\n", args->text, i);
            pthread_mutex_unlock(&args->sync->mutex_child);
        } else {
            pthread_mutex_lock(&args->sync->mutex_child);
            fprintf(stderr, "%s%d\n", args->text, i);
            pthread_mutex_unlock(&args->sync->mutex_parent);
        }
    }
}

void* launch_thread(void* arg) {
    thread_task((ThreadArgs*)arg);
    pthread_exit(NULL);
}

int main() {
    SyncContext sync;
    pthread_mutexattr_t attr;
    
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);

    pthread_mutex_init(&sync.mutex_parent, &attr);
    pthread_mutex_init(&sync.mutex_child, &attr);

    pthread_mutex_lock(&sync.mutex_parent);
    pthread_mutex_lock(&sync.mutex_child);

    pthread_t thread_id;

    ThreadArgs child_args = { &sync, "Child thread: string №", 0 };
    ThreadArgs parent_args = { &sync, "Parent thread: string №", 1 };

    int result;

    result = pthread_create(&thread_id, NULL, launch_thread, &child_args);

    if (result != 0) {
        fprintf(stderr, "Error while creating a thread: %s\n", strerror(result));
        exit(1);
    }

    pthread_mutex_unlock(&sync.mutex_parent);

    thread_task(&parent_args);

    pthread_join(thread_id, NULL);

    pthread_mutex_destroy(&sync.mutex_parent);
    pthread_mutex_destroy(&sync.mutex_child);
    pthread_mutexattr_destroy(&attr);
    
    return 0;
}

