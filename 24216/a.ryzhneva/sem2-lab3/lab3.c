#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#define COUNT_THREADS 4

void* thread_body(void* param) {
    const char** arr = (const char**)param;

    for (int i = 0; arr[i] != NULL; i++) {
        fprintf(stderr, "%s\n", arr[i]);
    }

    return NULL;
}

void check_code(int code, const char* name_prog, const char* action) {
    if (code != 0) {
        char buf[256];
        strerror_r(code, buf, sizeof buf);
        fprintf(stderr, "%s: %s: %s\n", name_prog, action, buf);
        exit(1);
    }
}

int main(int argc, char* argv[]) {
    pthread_t thread[COUNT_THREADS];
    int code;

    const char* set1[] = { "thread 1: 1", NULL };
    const char* set2[] = { "thread 2: 1", "thread 2: 2", NULL };
    const char* set3[] = { "thread 3: 1", "thread 3: 2", "thread 3: 3", NULL };
    const char* set4[] = { "thread 4: 1", "thread 4: 2", "thread 4: 3", "thread 4: 4", NULL };

    const char** arr[COUNT_THREADS] = { set1, set2, set3, set4 };

    for(int i = 0; i < COUNT_THREADS; i++) {
        code = pthread_create(&thread[i], NULL, thread_body, (void*)arr[i]);
        check_code(code, argv[0], "creating thread");
    }
    
    for (int i = 0; i < COUNT_THREADS; i++) {
        code = pthread_join(thread[i], NULL);
        check_code(code, argv[0], "joining thread");
    }

    return EXIT_SUCCESS;
}