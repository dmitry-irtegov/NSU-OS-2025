#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include <stdatomic.h>
#include <stdbool.h>

sem_t sem_a, sem_b, sem_c, sem_module;
atomic_bool running = true;

void check_error(int result, const char *msg) {
    if (result != 0) {
        fprintf(stderr, "%s: %s\n", msg, strerror(result));
        exit(1);
    }
}

void* produce_a(void* arg) {
    (void)arg;
    while (atomic_load(&running)) {
        sleep(1); 
        if (!atomic_load(&running)) break;
        fprintf(stderr, "Деталь A изготовлена\n");
        sem_post(&sem_a);
    }
    return NULL;
}

void* produce_b(void* arg) {
    (void)arg;
    while (atomic_load(&running)) {
        sleep(2);
        if (!atomic_load(&running)) break;
        fprintf(stderr, "Деталь B изготовлена\n");
        sem_post(&sem_b);
    }
    return NULL;
}

void* produce_c(void* arg) {
    (void)arg;
    while (atomic_load(&running)) {
        sleep(3);
        if (!atomic_load(&running)) break;
        fprintf(stderr, "Деталь C изготовлена\n");
        sem_post(&sem_c);
    }
    return NULL;
}

void* assemble_module(void* arg) {
    (void)arg;
    while (atomic_load(&running)) {
        if (sem_wait(&sem_a) != 0) break;
        if (!atomic_load(&running)) break;
        
        if (sem_wait(&sem_b) != 0) break;
        if (!atomic_load(&running)) break;

        fprintf(stderr, "Модуль собран из A и B\n");
        sem_post(&sem_module);
    }
    return NULL;
}

void* assemble_widget(void* arg) {
    int limit = *(int*)arg;
    for (int i = 1; i <= limit; i++) {
        if (sem_wait(&sem_module) != 0) break;
        if (sem_wait(&sem_c) != 0) break;
        fprintf(stderr, ">>> Винтик #%d готов!\n", i);
    }
    
    fprintf(stderr, "План выполнен: %d винтиков собрано\n", limit);
    
    atomic_store(&running, false);

    sem_post(&sem_a); 
    sem_post(&sem_b); 
    sem_post(&sem_c); 
    sem_post(&sem_module);
    
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Использование: %s <количество_винтиков>\n", argv[0]);
        return 1;
    }

    int target_count = atoi(argv[1]);
    if (target_count <= 0) {
        fprintf(stderr, "Ошибка: введите положительное число.\n");
        return 1;
    }

    sem_init(&sem_a, 0, 0);
    sem_init(&sem_b, 0, 0);
    sem_init(&sem_c, 0, 0);
    sem_init(&sem_module, 0, 0);

    pthread_t t_a, t_b, t_c, t_m, t_w;

    check_error(pthread_create(&t_a, NULL, produce_a, NULL), "pthread_create A");
    check_error(pthread_create(&t_b, NULL, produce_b, NULL), "pthread_create B");
    check_error(pthread_create(&t_c, NULL, produce_c, NULL), "pthread_create C");
    check_error(pthread_create(&t_m, NULL, assemble_module, NULL), "pthread_create Module");
    check_error(pthread_create(&t_w, NULL, assemble_widget, &target_count), "pthread_create Widget");

    check_error(pthread_join(t_w, NULL), "pthread_join Widget");

    pthread_join(t_a, NULL);
    pthread_join(t_b, NULL);
    pthread_join(t_c, NULL);
    pthread_join(t_m, NULL);

    sem_destroy(&sem_a);
    sem_destroy(&sem_b);
    sem_destroy(&sem_c);
    sem_destroy(&sem_module);

    fprintf(stderr, "Программа успешно завершена, ресурсы очищены.\n");
    return 0;
}
