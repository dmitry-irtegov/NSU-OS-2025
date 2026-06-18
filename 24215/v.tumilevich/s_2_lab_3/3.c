#include <stdio.h>
#include <pthread.h>

typedef struct {
    char **all_strings;
    int start;
    int end;
} ThreadArgs;

void* print_strings(void* arg) {
    ThreadArgs* data = (ThreadArgs*)arg;
    pthread_t id = pthread_self();

    for (int i = data->start; i < data->end; i++) {
        printf("Поток %lx: %s\n", (unsigned long)id, data->all_strings[i]);
    }

    return NULL;
}

int main() {
    const int num_threads = 4;
    
    char *shared_seq[] = {
        "1", "2", "3", "4",
        "5", "6", "7", "8",
        "9", "10", "11"
    };

    int total_strings = sizeof(shared_seq) / sizeof(shared_seq[0]);

    pthread_t threads[num_threads];
    ThreadArgs args[num_threads];

    int base_count = total_strings / num_threads;
    int remainder = total_strings % num_threads;
    
    int current_start = 0;

    for (int i = 0; i < num_threads; i++) {
        args[i].all_strings = shared_seq;
        args[i].start = current_start;
        
        int count_for_this_thread = base_count + (i < remainder ? 1 : 0);
        args[i].end = current_start + count_for_this_thread;
        
        current_start = args[i].end;

        if (pthread_create(&threads[i], NULL, print_strings, &args[i]) != 0) {
            fprintf(stderr, "Ошибка при создании потока %d\n", i);
            return 1;
        }
    }

    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("all\n");
    return 0;
}