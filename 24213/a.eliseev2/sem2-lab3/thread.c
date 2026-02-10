#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

const char *(strings[][4]) = {
    {
        "Thread 1 foo\n",
        "Thread 1 bar\n",
        "Thread 1 baz\n",
        NULL,
    },
    {
        "Thread 2 foo\n",
        "Thread 2 bar\n",
        "Thread 2 baz\n",
        NULL,
    },
    {
        "Thread 3 foo\n",
        "Thread 3 bar\n",
        "Thread 3 baz\n",
        NULL,
    },
    {
        "Thread 4 foo\n",
        "Thread 4 bar\n",
        "Thread 4 baz\n",
        NULL,
    },
};

void *thread_run(void *arg) {
    char **strings = (char **)arg;
    for (char **string = strings; *string != NULL; string++) {
        printf("%s", *string);
    }
    pthread_exit(NULL);
}

int main() {
    for (int i = 0; i < 4; i++) {
        pthread_t pthread;
        int error = pthread_create(&pthread, NULL, thread_run, strings[i]);
        if (error) {
            fprintf(stderr, "could not create thread: %s\n", strerror(error));
            return 1;
        }
    }
    pthread_exit(NULL);
}
