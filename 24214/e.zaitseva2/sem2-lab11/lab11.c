#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>

#define LINES 10
#define BUF_SIZE 256


static atomic_int child_ready = 0;
static pthread_mutex_t mutexes[3];

void check_error(int error, const char *context) {
    if (error != 0) {
        char err_msg[BUF_SIZE];
        strerror_r(error, err_msg, sizeof(err_msg));
        fprintf(stderr, "%s: %s\n", context, err_msg);
        exit(EXIT_FAILURE);
    }
}

void print_line(const char *prefix, pthread_mutex_t *mutexes, int mut_index) {
    for (int i = 0; i < LINES; i++) {
        for (int j = 0; j < 3; j++) {
            if (mut_index % 3 == 0) {
                fprintf(stderr, "%s line %d\n", prefix, i + 1);
            }

            int error;
            error = pthread_mutex_lock(&mutexes[(mut_index + 1) % 3]);
            check_error(error, "Error mutex lock");

            error = pthread_mutex_unlock(&mutexes[mut_index % 3]);
            check_error(error, "Error mutex unlock");

            mut_index++;
        }
    }
}

void *thread_run(void *_arg) {
    (void)_arg;

    check_error(
        pthread_mutex_lock(&mutexes[1]),
        "Error lock mutex in child thread"
    );

    atomic_store(&child_ready, 1);

    print_line("Child", mutexes, 1);

    return NULL;
}

int main(void) {
    pthread_t thread;
    pthread_mutexattr_t attr;

    check_error(
        pthread_mutexattr_init(&attr),
        "Error init mutex attr"
    );

    check_error(
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK),
        "Error set mutex attr"
    );

    for (int i = 0; i < 3; i++) {
        check_error(
            pthread_mutex_init(&mutexes[i], &attr),
            "Error init mutex"
        );
    }

    check_error(
        pthread_mutexattr_destroy(&attr),
        "Error destroy mutex attr"
    );

    check_error(
        pthread_mutex_lock(&mutexes[0]),
        "Error lock mutex in parent"
    );

    check_error(
        pthread_create(&thread, NULL, thread_run, NULL),
        "Error creat thread"
    );

    while (!atomic_load(&child_ready)) {
    }

    print_line("Parent", mutexes, 0);

    check_error(
        pthread_join(thread, NULL),
        "Error join thread"
    );

    for (int i = 0; i < 3; i++) {
        check_error(
            pthread_mutex_destroy(&mutexes[i]), 
            "Error mutex destroy"
        );
    }

    return EXIT_SUCCESS;
}