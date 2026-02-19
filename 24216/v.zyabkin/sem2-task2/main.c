#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

void thread_task(char *str) {
    for (int i = 1; i <= 10; i++) {
        fprintf(stderr, "%s%d\n", str, i);
    }
}

void* launch_thread(void* arg) {
    thread_task((char*)arg);
    pthread_exit(NULL);
}

int main() {
    pthread_t thread_id;
    int result;

    result = pthread_create(&thread_id, NULL, launch_thread, "Child thread: string №");

    if (result != 0) {
        fprintf(stderr, "Error while creating a thread: %d\n", result);
        exit(1);
    }

    pthread_join(thread_id, NULL);

    thread_task("Parent thread: string №");

    pthread_exit(NULL);
}