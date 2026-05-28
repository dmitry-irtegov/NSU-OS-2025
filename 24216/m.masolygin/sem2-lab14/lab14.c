#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

sem_t sem_parent;
sem_t sem_child;

void sem_init_checked(sem_t* sem, unsigned int value) {
    if (sem_init(sem, 0, value) != 0) {
        fprintf(stderr, "Error initializing semaphore: %s\n", strerror(errno));
        exit(1);
    }
}

void sem_wait_checked(sem_t* sem) {
    if (sem_wait(sem) != 0) {
        fprintf(stderr, "Error waiting on semaphore: %s\n", strerror(errno));
        exit(1);
    }
}

void sem_post_checked(sem_t* sem) {
    if (sem_post(sem) != 0) {
        fprintf(stderr, "Error posting semaphore: %s\n", strerror(errno));
        exit(1);
    }
}

void sem_destroy_checked(sem_t* sem) {
    if (sem_destroy(sem) != 0) {
        fprintf(stderr, "Error destroying semaphore: %s\n", strerror(errno));
        exit(1);
    }
}

void* printer_ten(void* arg) {
    (void)arg;
    for (int i = 0; i < 10; i++) {
        sem_wait_checked(&sem_child);

        fprintf(stderr, "Child string: %d\n", i + 1);

        sem_post_checked(&sem_parent);
    }

    return NULL;
}

int main() {
    sem_init_checked(&sem_parent, 1);
    sem_init_checked(&sem_child, 0);

    pthread_t thread;
    int error = pthread_create(&thread, NULL, printer_ten, NULL);
    if (error != 0) {
        fprintf(stderr, "Error creating threads: %s\n", strerror(error));
        return 1;
    }

    for (int i = 0; i < 10; i++) {
        sem_wait_checked(&sem_parent);

        fprintf(stderr, "Parent string: %d\n", i + 1);

        sem_post_checked(&sem_child);
    }

    error = pthread_join(thread, NULL);
    if (error != 0) {
        fprintf(stderr, "Error joining thread: %s\n", strerror(error));
        return 1;
    }

    sem_destroy_checked(&sem_parent);
    sem_destroy_checked(&sem_child);

    return 0;
}
