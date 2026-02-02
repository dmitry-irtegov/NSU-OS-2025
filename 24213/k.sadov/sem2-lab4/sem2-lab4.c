#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void *thread_body(void *param) {
    while (1) {
        printf("child: running...\n");
        sleep(1);
    }
}

int main(int argc, char *argv[]) {
    pthread_t thread;
    int code;

    code = pthread_create(&thread, NULL, thread_body, NULL);
    if (code != 0) {
        char buf[256];
        strerror_r(code, buf, sizeof buf);
        fprintf(stderr, "%s: creating thread: %s\n", argv[0], buf);
        exit(EXIT_FAILURE);
    }

    sleep(2);

    pthread_cancel(thread);

    pthread_join(thread, NULL);
    printf("parent: cancelled\n");

    return (EXIT_SUCCESS);
}
