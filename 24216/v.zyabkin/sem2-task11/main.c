#include <pthread.h>
#include <sched.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct SafeSync {
    pthread_mutex_t m[3];
    atomic_int child_start;
} SyncContext;

typedef struct ThreadArgs {
    SyncContext* sync;
    char* text;
    int is_parent; // 1 - parent's turn, 0 - child's turn
} ThreadArgs;

void thread_task(ThreadArgs* args) {
    if (!args->is_parent) {
        pthread_mutex_lock(&args->sync->m[2]);
        atomic_store(&args->sync->child_start, 1);
    }

    for (int i = 0; i < 100; i++) {
        if (args->is_parent) {
            pthread_mutex_lock(&args->sync->m[(i + 1) % 3]);
            fprintf(stderr, "%s%d\n", args->text, i + 1);
            pthread_mutex_unlock(&args->sync->m[(i + 0) % 3]);
        } else {
            pthread_mutex_lock(&args->sync->m[(i + 0) % 3]);
            fprintf(stderr, "%s%d\n", args->text, i + 1);
            pthread_mutex_unlock(&args->sync->m[(i + 2) % 3]);
        }
    }

    if (args->is_parent) {
        pthread_mutex_unlock(&args->sync->m[(99 + 1) % 3]);
    } else {
        pthread_mutex_unlock(&args->sync->m[(99 + 0) % 3]);
    }
}

void* launch_thread(void* arg) {
    thread_task((ThreadArgs*)arg);
    pthread_exit(NULL);
}

int main() {
    SyncContext sync;
    atomic_init(&sync.child_start, 0);
    pthread_mutexattr_t attr;
    
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);

    for (int i = 0; i < 3; i++) {
        if (pthread_mutex_init(&sync.m[i], &attr) != 0) {
            fprintf(stderr, "Error initializing mutex %d\n", i);
            exit(1);
        }
    }

    pthread_mutex_lock(&sync.m[0]);

    pthread_t thread_id;

    ThreadArgs child_args = { &sync, "Child thread: string №", 0 };
    ThreadArgs parent_args = { &sync, "Parent thread: string №", 1 };

    int result;
    result = pthread_create(&thread_id, NULL, launch_thread, &child_args);

    if (result != 0) {
        fprintf(stderr, "Error while creating a thread: %s\n", strerror(result));
        exit(1);
    }

    while (!atomic_load(&sync.child_start)) {
        sched_yield();
    }

    thread_task(&parent_args);

    pthread_join(thread_id, NULL);

    for (int i = 0; i < 3; i++) {
        pthread_mutex_destroy(&sync.m[i]);
    }
    pthread_mutexattr_destroy(&attr);
    
    return 0;
}