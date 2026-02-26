#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

const char* string1[] = {"First: I\n", "First: love\n", "First: OSI\n", NULL};
const char* string2[] = {"Second: I\n", "Second: love\n", "Second: OSI\n", NULL};
const char* string3[] = {"Third: I\n", "Third: love\n", "Third: OSI\n", NULL};
const char* string4[] = {"Fourth: I\n", "Fourth: love\n", "Fourth: OSI\n", NULL};

const char** strings[] = {string1, string2, string3, string4};

void* thread_printer(void* arg) {
    const char** lines = (const char** )arg;

    for (int i = 0; lines[i] != NULL; i++) {
        fprintf(stderr, "%s", lines[i]);
    }

    return NULL;
}

int main() {
    pthread_t workers[4];

    for (int i = 0; i < 4; i++) {
        int status = pthread_create(&workers[i], NULL, thread_printer, (void* )strings[i]);
        
        if (status != 0) {
            fprintf(stderr, "Error: pthread_create: %s", strerror(status));
            return 1;
        }
    }

    for (int i = 0; i < 4; i++) {
        pthread_join(workers[i], NULL);
    }

    return 0;
}