#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

void *foo_thread() {
    int i = 1;
    while (i <= 10) {
        printf("thead line %d\n", i);
        i++;
    }
    return NULL;
}

int main() {
    pthread_t thread;

    if (pthread_create(&thread, NULL, foo_thread, NULL) != 0){
        printf("Error with thread creating");
        return EXIT_FAILURE;
    }


    for (int i = 1; i <= 10; i++) {
        printf("parent line %d\n", i);
    }

    pthread_join(thread, NULL);

    return 0;
}