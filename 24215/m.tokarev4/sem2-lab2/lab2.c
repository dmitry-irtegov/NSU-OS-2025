#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

void* print10lines(void* arg) {
    char* text = (char*)arg;
    for (int i = 1; i <= 10; i++) {
        printf("%d - %s\n", i, text);
    }
    return NULL;
}

int main() {
    pthread_t thread;
    int status;

    status = pthread_create(&thread, NULL, print10lines, "D");
    if (status != 0) {
        fprintf(stderr, "Error creating thread\n");
        return 1;
   }

    print10lines("P");

    pthread_join(thread, NULL);

    return 0;
}
