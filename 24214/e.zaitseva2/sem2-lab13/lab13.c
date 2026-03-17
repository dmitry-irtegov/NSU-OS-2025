#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LINES 10
#define BUF_SIZE 256

typedef enum {
    TURN_PARENT,
    TURN_CHILD
} turn_t;

static pthread_mutex_t mutex;
static pthread_cond_t cond;
static turn_t turn = TURN_PARENT;

void check_error(int error, const char *context) {
    if (error != 0) {
        char err_msg[BUF_SIZE];
        strerror_r(error, err_msg, sizeof(err_msg));
        fprintf(stderr, "%s: %s\n", context, err_msg);
        exit(EXIT_FAILURE);
    }
}

void *thread_run(void *arg) {
    (void)arg;

    for (int i = 0; i < LINES; i++) {
        check_error(
            pthread_mutex_lock(&mutex),
            "Error lock mutex in child"
        );

        while (turn != TURN_CHILD) {
            check_error(
                pthread_cond_wait(&cond, &mutex),
                "Error wait condition in child"
            );
        }

        fprintf(stderr, "Child line %d\n", i + 1);

        turn = TURN_PARENT;

        check_error(
            pthread_cond_signal(&cond),
            "Error signal condition in child"
        );

        check_error(
            pthread_mutex_unlock(&mutex),
            "Error unlock mutex in child"
        );
    }

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

    check_error(
        pthread_mutex_init(&mutex, &attr),
        "Error init mutex"
    );

    check_error(
        pthread_mutexattr_destroy(&attr),
        "Error destroy mutex attr"
    );

    check_error(
        pthread_cond_init(&cond, NULL),
        "Error init condition variable"
    );

    check_error(
        pthread_create(&thread, NULL, thread_run, NULL),
        "Error create thread"
    );

    for (int i = 0; i < LINES; i++) {
        check_error(
            pthread_mutex_lock(&mutex),
            "Error lock mutex in parent"
        );

        while (turn != TURN_PARENT) {
            check_error(
                pthread_cond_wait(&cond, &mutex),
                "Error wait condition in parent"
            );
        }

        fprintf(stderr, "Parent line %d\n", i + 1);

        turn = TURN_CHILD;

        check_error(
            pthread_cond_signal(&cond),
            "Error signal condition in parent"
        );

        check_error(
            pthread_mutex_unlock(&mutex),
            "Error unlock mutex in parent"
        );
    }

    check_error(
        pthread_join(thread, NULL),
        "Error join thread"
    );

    check_error(
        pthread_cond_destroy(&cond),
        "Error destroy condition variable"
    );

    check_error(
        pthread_mutex_destroy(&mutex),
        "Error destroy mutex"
    );

    return EXIT_SUCCESS;
}