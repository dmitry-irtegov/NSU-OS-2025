#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#define MAX_LINE_LEN 4096
#define CHUNK_SIZE 80

#define NUM_SORT_THREADS 2

typedef struct Node {
    char str[CHUNK_SIZE + 1];
    pthread_mutex_t mutex;
    struct Node* next;
} Node;

typedef struct {
    Node head; 
    int is_running;
} AppContext;

typedef struct {
    AppContext* ctx;
    int thread_id;
    int interval_sec;
} ThreadData;

void push_front(AppContext* ctx, const char* str) {
    Node* new_node = (Node*)malloc(sizeof(Node));
    if (!new_node) {
        perror("Ошибка выделения памяти");
        exit(EXIT_FAILURE);
    }
    strncpy(new_node->str, str, CHUNK_SIZE);
    new_node->str[CHUNK_SIZE] = '\0';
    pthread_mutex_init(&new_node->mutex, NULL);
    
    pthread_mutex_lock(&ctx->head.mutex);
    new_node->next = ctx->head.next;
    ctx->head.next = new_node;
    pthread_mutex_unlock(&ctx->head.mutex);
}

void print_list(AppContext* ctx) {
    printf("\n--------------------------------\n");
    Node* prev = &ctx->head;
    pthread_mutex_lock(&prev->mutex);
    
    Node* curr = prev->next;
    int index = 1;
    
    while (curr != NULL) {
        pthread_mutex_lock(&curr->mutex);
        printf("[%d]: %s\n", index++, curr->str);
        
        Node* next_node = curr->next;
        pthread_mutex_unlock(&prev->mutex);
        
        prev = curr;
        curr = next_node;
    }
    pthread_mutex_unlock(&prev->mutex);
    printf("--------------------------------\n\n");
}

void bubble_sort(AppContext* ctx) {
    int swapped;
    do {
        sleep(1);       //19 задача только тут измение

        swapped = 0;
        
        Node* prev = &ctx->head;
        pthread_mutex_lock(&prev->mutex);
        
        Node* curr = prev->next;
        if (curr == NULL) {
            pthread_mutex_unlock(&prev->mutex);
            break;
        }
        pthread_mutex_lock(&curr->mutex);
        
        while (curr->next != NULL && ctx->is_running) {
            Node* next_node = curr->next;
            pthread_mutex_lock(&next_node->mutex);
            
            if (strcmp(curr->str, next_node->str) > 0) {
                curr->next = next_node->next;
                next_node->next = curr;
                prev->next = next_node;
                
                swapped = 1;

                Node* old_prev = prev;
                prev = next_node;
                pthread_mutex_unlock(&old_prev->mutex);
            } else {
                Node* old_prev = prev;
                prev = curr;
                curr = next_node;
                pthread_mutex_unlock(&old_prev->mutex);
            }
        }
        
        if (curr != NULL) pthread_mutex_unlock(&curr->mutex);
        if (prev != NULL) pthread_mutex_unlock(&prev->mutex);
        
    } while (swapped && ctx->is_running);
}

void* sort_thread_func(void* arg) {
    ThreadData* td = (ThreadData*)arg;
    while (1) {
        sleep(td->interval_sec); 
        if (!td->ctx->is_running) break;
        bubble_sort(td->ctx);
    }
    return NULL;
}

int main() {
    char buffer[MAX_LINE_LEN];
    AppContext ctx;
    ctx.head.next = NULL;
    pthread_mutex_init(&ctx.head.mutex, NULL);
    ctx.is_running = 1;

    pthread_t sort_threads[NUM_SORT_THREADS];
    ThreadData thread_data[NUM_SORT_THREADS];

    for (int i = 0; i < NUM_SORT_THREADS; ++i) {
        thread_data[i].ctx = &ctx;
        thread_data[i].thread_id = i + 1;
        thread_data[i].interval_sec = (i == 0) ? 3 : 5; 
        
        if (pthread_create(&sort_threads[i], NULL, sort_thread_func, &thread_data[i]) != 0) {
            perror("Ошибка создания потока");
            return EXIT_FAILURE;
        }
    }

    while (fgets(buffer, sizeof(buffer), stdin) != NULL) {
        size_t len = strlen(buffer);
        
        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
            len--;
        }

        if (len == 0) {
            print_list(&ctx);
        } else {
            for (size_t i = 0; i < len; i += CHUNK_SIZE) {
                char chunk[CHUNK_SIZE + 1] = {0};
                strncpy(chunk, buffer + i, CHUNK_SIZE);
                push_front(&ctx, chunk);
            }
        }
    }

    ctx.is_running = 0;
    for (int i = 0; i < NUM_SORT_THREADS; ++i) {
        pthread_join(sort_threads[i], NULL);
    }

    Node* current = ctx.head.next;
    while (current != NULL) {
        Node* next = current->next;
        pthread_mutex_destroy(&current->mutex);
        free(current);
        current = next;
    }
    pthread_mutex_destroy(&ctx.head.mutex);

    return EXIT_SUCCESS;
}