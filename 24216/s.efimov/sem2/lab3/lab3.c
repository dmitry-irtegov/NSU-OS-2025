#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char **sentences;
    int count;
    int thread_num;
} thread_data_t;

void* print_strings(void* arg) {
    thread_data_t *data = (thread_data_t*)arg;
    
    for (int i = 0; i < data->count; i++) {
        printf("Поток %d: %s\n", data->thread_num, data->sentences[i]);
    }
    
    pthread_exit(NULL);
}

int main() {
    pthread_t threads[4];
    thread_data_t t_data[4];

    char *list0[] = {"Яблоко", "Груша"};
    char *list1[] = {"Кот", "Собака", "Рыба"};
    char *list2[] = {"Чёрный", "Красный", "Белый", "Синий"};
    char *list3[] = {"Понедельник", "Вторник"};

    char **all_lists[] = {list0, list1, list2, list3};
    int counts[] = {2, 3, 4, 2};

    for (int i = 0; i < 4; i++) {
        t_data[i].sentences = all_lists[i];
        t_data[i].count = counts[i];
        t_data[i].thread_num = i + 1;

        int status = pthread_create(&threads[i], NULL, print_strings, &t_data[i]);
        if (status != 0) {
            fprintf(stderr, "Ошибка создания потока %d: %s\n", i, strerror(status));
            break; 
        }
    }

    for (int i = 0; i < 4; i++) {
        int status = pthread_join(threads[i], NULL);
        if (status != 0) {
            fprintf(stderr, "Ошибка join для потока %d: %s\n", i, strerror(status));
        }
    }

    printf("Все потоки завершили работу.\n");
    return 0;
}