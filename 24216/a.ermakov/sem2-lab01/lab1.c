#include <stdio.h>
#include <pthread.h>

void* print10Str(void* arg) {
    (void)arg;
    for (int i = 1; i <= 10; i++) {
        printf("Дочерняя нить: строка %d\n", i);
    }
    return NULL;
}

int main() {
    pthread_t thr;

    if (pthread_create(&thr, NULL, print10Str, NULL) != 0) {
        perror("Error creating thread");
        return 1;
    }

    for (int i = 1; i <= 10; i++) {
        printf("Родительская нить: строка %d\n", i);
    }

    pthread_join(thr, NULL);
  
    return 0;
}
