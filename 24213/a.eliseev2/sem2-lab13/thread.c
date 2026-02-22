#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ITERATIONS 10
#define STRERROR_BUF_SIZE 256

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
static bool turn = false;

void print_alternating(const char *message, bool my_turn) {
    int error;
    char err_msg[STRERROR_BUF_SIZE] = "(error message too long)";

    for (int i = 0; i < 10; i++) {
        if ((error = pthread_mutex_lock(&mutex))) {
            strerror_r(error, err_msg, sizeof(err_msg));
            fprintf(stderr, "could not lock mutex: %s\n", err_msg);
            exit(1);
        }

        if (turn != my_turn) {
            if ((error = pthread_cond_wait(&cond, &mutex))) {
                strerror_r(error, err_msg, sizeof(err_msg));
                fprintf(stderr, "could not wait: %s\n", err_msg);
                exit(1);
            }
        }
        turn = !my_turn;

        printf("%s", message);
        if ((error = pthread_mutex_unlock(&mutex))) {
            strerror_r(error, err_msg, sizeof(err_msg));
            fprintf(stderr, "could not unlock mutex: %s\n", err_msg);
            exit(1);
        }

        if ((error = pthread_cond_signal(&cond))) {
            strerror_r(error, err_msg, sizeof(err_msg));
            fprintf(stderr, "could not signal: %s\n", err_msg);
            exit(1);
        }
    }
}

void *thread_run(void *arg) {
    print_alternating("I'm alive (child)\n", true);
    return NULL;
}

int main() {
    int error;
    char err_msg[STRERROR_BUF_SIZE] = "(error message too long)";

    pthread_t pthread;
    if ((error = pthread_create(&pthread, NULL, thread_run, NULL))) {
        strerror_r(error, err_msg, sizeof(err_msg));
        fprintf(stderr, "could not create thread: %s\n", err_msg);
        exit(1);
    }

    print_alternating("I'm alive (parent)\n", false);

    if ((error = pthread_join(pthread, NULL))) {
        strerror_r(error, err_msg, sizeof(err_msg));
        fprintf(stderr, "could not join thread: %s\n", err_msg);
        exit(1);
    }
    return 0;
}
