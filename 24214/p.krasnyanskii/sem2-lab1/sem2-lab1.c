#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

void* child_thread(void* arg) {
    for (int i = 0; i < 10; i++) {
        printf("Child thread: %d\n", i + 1);
        usleep(100000);
    }
    return NULL;
}

int main() {
    pthread_t tid;

    if (pthread_create(&tid, NULL, child_thread, NULL) != 0) {
        perror("pthread_create");
        return 1;
    }

    for (int i = 0; i < 10; i++) {
        printf("Parent thread: %d\n", i + 1);
        usleep(100000);
    }

    pthread_join(tid, NULL);

    return 0;
}

