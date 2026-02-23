#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct SafeSync {
    pthread_mutex_t m;
    pthread_cond_t allow;
    int current_turn; // 0 - parent, 1 - child
} SafeSync;

typedef struct ThreadArgs {
    SafeSync* sync;
    char* text;
    int my_id;
} ThreadArgs;

void thread_task(ThreadArgs* args) {
    for (int i = 1; i <= 100; i++) {
        pthread_mutex_lock(&args->sync->m);

        while (args->sync->current_turn != args->my_id) {
            pthread_cond_wait(&args->sync->allow, &args->sync->m);
        }

        fprintf(stderr, "%s%d\n", args->text, i);

        args->sync->current_turn = (args->my_id == 0) ? 1 : 0;

        pthread_cond_signal(&args->sync->allow);
        pthread_mutex_unlock(&args->sync->m);
    }
}

void* launch_thread(void* arg) {
    thread_task((ThreadArgs*)arg);
    pthread_exit(NULL);
}

int main() {
    SafeSync sync;
    pthread_mutexattr_t attr;
    
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
    
    pthread_mutex_init(&sync.m, &attr);
    pthread_cond_init(&sync.allow, NULL);
    sync.current_turn = 0; // parent at first

    ThreadArgs child_args = { &sync, "Child thread: string №", 1 };
    ThreadArgs parent_args = { &sync, "Parent thread: string №", 0 };

    pthread_t thread_id;
    int result;

    result = pthread_create(&thread_id, NULL, launch_thread, &child_args);

    if (result != 0) {
        fprintf(stderr, "Error while creating a thread: %s\n", strerror(result));
        exit(1);
    }

    thread_task(&parent_args);

    pthread_join(thread_id, NULL);

    pthread_mutexattr_destroy(&attr);
    pthread_mutex_destroy(&sync.m);
    pthread_cond_destroy(&sync.allow);

    return 0;
}

