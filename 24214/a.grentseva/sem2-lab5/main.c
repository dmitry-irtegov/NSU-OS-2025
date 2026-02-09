#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>

void cleanup_handler() {
    printf("child thread is cleaning up before exit\n");
}

void* child_thread() {
    pthread_cleanup_push(cleanup_handler, NULL);
    while (1) {
        printf("child thread is writing\n");
        sleep(1);   
    }
    pthread_cleanup_pop(0); 
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

