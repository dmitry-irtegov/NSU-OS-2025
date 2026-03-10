#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ITERATIONS 10
#define STRERROR_BUF_SIZE 256

static volatile char child_ready = 0;
static pthread_mutex_t mutexes[3] = {
    PTHREAD_MUTEX_INITIALIZER,
    PTHREAD_MUTEX_INITIALIZER,
    PTHREAD_MUTEX_INITIALIZER,
};

int print_alternating(char *message, pthread_mutex_t *mutexes, int mut_index) {
    for (int i = 0; i < ITERATIONS; i++) {
        for (int j = 0; j < 3; j++) {
            if (mut_index % 3 == 0) {
                printf("%s", message);
            }
            int error;
            if ((error = pthread_mutex_lock(&mutexes[(mut_index + 1) % 3]))) {
                return error;
            }
            if ((error = pthread_mutex_unlock(&mutexes[mut_index % 3]))) {
                return error;
            }
            mut_index++;
        }
    }
    return 0;
}

void *thread_run(void *_arg) {
    int error;
    char err_msg[STRERROR_BUF_SIZE] = "(error message too long)";

    if ((error = pthread_mutex_lock(&mutexes[1]))) {
        strerror_r(error, err_msg, sizeof(err_msg));
        fprintf(stderr, "could not lock mutex in child: %s\n", err_msg);
        exit(1);
    }

    child_ready = 1;

    if ((error = print_alternating("I'm alive (child)\n", mutexes, 1))) {
        strerror_r(error, err_msg, sizeof(err_msg));
        fprintf(stderr, "error while printing in child: %s\n", err_msg);
        exit(1);
    }

    return NULL;
}

int main() {
    int error;
    char err_msg[STRERROR_BUF_SIZE] = "(error message too long)";

    if ((error = pthread_mutex_lock(&mutexes[0]))) {
        strerror_r(error, err_msg, sizeof(err_msg));
        fprintf(stderr, "could not lock mutex in parent: %s\n", err_msg);
        exit(1);
    }

    pthread_t pthread;

    if ((error = pthread_create(&pthread, NULL, thread_run, NULL))) {
        strerror_r(error, err_msg, sizeof(err_msg));
        fprintf(stderr, "could not create thread: %s\n", err_msg);
        exit(1);
    }

    while (!child_ready) {
    }
    
    if ((error = print_alternating("I'm alive (parent)\n", mutexes, 0))) {
        strerror_r(error, err_msg, sizeof(err_msg));
        fprintf(stderr, "error while printing in parent: %s\n", err_msg);
        exit(1);
    }

    if ((error = pthread_join(pthread, NULL))) {
        strerror_r(error, err_msg, sizeof(err_msg));
        fprintf(stderr, "could not join thread: %s\n", err_msg);
        exit(1);
    }

    return 0;
}
