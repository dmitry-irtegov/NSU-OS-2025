#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

void* child_thread(void* arg) {
    for (int i = 0; i < 10; i++) {
        printf("Child thread: %d\n", i);
    }
    return NULL;
}

int main() {
    pthread_t tid;

    if (pthread_create(&tid, NULL, child_thread, NULL) != 0) {
        perror("pthread_create");
        return 1;
    }

    pthread_join(tid, NULL);

    for (int i = 0; i < 10; i++) {
        printf("Parent thread: %d\n", i);
    }

    return 0;
}

