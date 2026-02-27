#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct package_strings {
    int cnt;
    char** strings;
} package_strings_t;

void print_lines(package_strings_t* package) {
    for (int i = 0; i < package->cnt; i++) {
        fprintf(stderr, "%s", package->strings[i]);
    }
}

void* printer_ten(void* arg) {
    print_lines((package_strings_t*)arg);
    pthread_exit(NULL);
}

void create_thread(pthread_t* thread, void* (*start_routine)(void*),
                   void* arg) {
    int error = pthread_create(thread, NULL, start_routine, arg);

    if (error != 0) {
        fprintf(stderr, "Error creating threads: %s\n", strerror(error));
        exit(1);
    }
}

int main() {
    pthread_t threads[4];

    package_strings_t package0 = {
        .cnt = 3,
        .strings = (char*[]){"Thread 0: Line 1\n", "Thread 0: Line 2\n",
                             "Thread 0: Line 3\n"},
    };

    package_strings_t package1 = {
        .cnt = 3,
        .strings = (char*[]){"Thread 1: Line 1\n", "Thread 1: Line 2\n",
                             "Thread 1: Line 3\n"},
    };

    package_strings_t package2 = {
        .cnt = 3,
        .strings = (char*[]){"Thread 2: Line 1\n", "Thread 2: Line 2\n",
                             "Thread 2: Line 3\n"},
    };

    package_strings_t package3 = {
        .cnt = 3,
        .strings = (char*[]){"Thread 3: Line 1\n", "Thread 3: Line 2\n",
                             "Thread 3: Line 3\n"},
    };

    create_thread(&threads[0], printer_ten, &package0);
    create_thread(&threads[1], printer_ten, &package1);
    create_thread(&threads[2], printer_ten, &package2);
    create_thread(&threads[3], printer_ten, &package3);

    for (int i = 0; i < 4; i++) {
        int error = pthread_join(threads[i], NULL);
        if (error != 0) {
            fprintf(stderr, "Error joining thread %d: %s\n", i,
                    strerror(error));
            exit(1);
        }
    }

    return 0;
}
