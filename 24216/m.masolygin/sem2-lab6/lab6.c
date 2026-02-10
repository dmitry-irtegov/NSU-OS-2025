#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_STRINGS 100
#define SLEEP_FACTOR 20000

void* sleep_sort_thread(void* arg) {
    char* str = (char*)arg;

    usleep(strlen(str) * SLEEP_FACTOR);
    write(STDOUT_FILENO, str, strlen(str));
    free(str);
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

void join_thread(pthread_t* thread) {
    int error = pthread_join(*thread, NULL);

    if (error != 0) {
        fprintf(stderr, "Error joining threads: %s\n", strerror(error));
        exit(1);
    }
}

int main(int argc, char* argv[]) {
    FILE* input = stdin;

    if (argc == 2) {
        input = fopen(argv[1], "r");
        if (input == NULL) {
            fprintf(stderr, "Error opening file\n");
            return 1;
        }
    } else {
        fprintf(stderr, "Usage: %s [input_file]\n", argv[0]);
        return 1;
    }

    pthread_t threads[MAX_STRINGS];

    char* string = NULL;
    size_t len = 0;
    int count = 0;

    while (getline(&string, &len, input) != -1 && count < MAX_STRINGS) {
        char* thread_string = strdup(string);

        if (thread_string == NULL) {
            fprintf(stderr, "Error duplicating string\n");
            free(string);
            break;
        }

        create_thread(&threads[count], sleep_sort_thread, thread_string);
        count++;
    }

    free(string);
    if (input != stdin) {
        fclose(input);
    }

    for (int i = 0; i < count; i++) {
        join_thread(&threads[i]);
    }

    return 0;
}
