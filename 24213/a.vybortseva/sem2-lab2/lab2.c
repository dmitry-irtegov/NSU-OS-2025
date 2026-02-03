#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

void* thread_body(void* arg) {
    for (int i = 1; i <= 10; i++) {
        printf("Child: %d\n", i);
    }
    return NULL;
}

int main(int argc, char* argv[]) {
    pthread_t thread;

    int code = pthread_create(&thread, NULL, thread_body, NULL);
    if (code != 0) {
        char buf[256];
        strerror_r(code, buf, sizeof(buf));
        fprintf(stderr, "%s: creating thread: %s\n", argv[0], buf);
        return -1;
    }

    pthread_join(thread, NULL);

    for (int i = 1; i <= 10; i++) {
        printf("Parent: %d\n", i);
    }

    return 0;
}
