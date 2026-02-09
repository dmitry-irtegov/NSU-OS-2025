#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>

void* child_thread() {
    while (1) {
        printf("child thread is writing\n");
        sleep(1);  
    }
    return NULL;
}

int main() {
    pthread_t tid;
    int ret;

    ret = pthread_create(&tid, NULL, child_thread, NULL);
    if (ret != 0) {
        perror("pthread_create failed");
        return 1;
    }

    sleep(2);

    printf("parent thread is stopping child thread\n");

    ret = pthread_cancel(tid);
    if (ret != 0) {
        perror("pthread_cancel failed");
        return 1;
    }

    ret = pthread_join(tid, NULL);
    if (ret != 0) {
        perror("pthread_join failed");
        return 1;
    }

    printf("child thread stopped\n");
    return 0;
}
