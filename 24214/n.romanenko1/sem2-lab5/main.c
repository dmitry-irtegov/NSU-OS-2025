#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

static void cleanup_handler(void *arg) {
    (void)arg;
    printf("Child thread is finishing...\n");
}

static void *thread_func(void *arg) {
    (void)arg;
    pthread_cleanup_push(cleanup_handler, NULL);

    while (1) {
        printf("Child thread is running...\n");
        sleep(1);
    }

    pthread_cleanup_pop(1);
    return NULL;
}

int main(void) {
    pthread_t thread;

    if (pthread_create(&thread, NULL, thread_func, NULL) != 0) {
        perror("pthread_create");
        return 1;
    }

    sleep(2);

    pthread_cancel(thread);
    pthread_join(thread, NULL);

    printf("Child thread cancelled.\n");
    return 0;
}
