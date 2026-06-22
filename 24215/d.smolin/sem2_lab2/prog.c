#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

void* child_thread(void* arg) {
    for(int i = 0; i < 10; i++) {
        printf("Новая нить: строка %d\n", i + 1);
    }
    return NULL;
}

int main() {
    pthread_t thread;
    pthread_create(&thread, NULL, child_thread, NULL);
    pthread_join(thread, NULL);
    for(int i = 0; i < 10; i++) {
        printf("Родительская нить: строка %d\n", i + 1);
    }
    return 0;
}
