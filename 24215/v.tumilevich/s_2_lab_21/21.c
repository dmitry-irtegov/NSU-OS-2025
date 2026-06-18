#define _GNU_SOURCE

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
    pthread_rwlock_t rwlock;
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
    pthread_rwlock_init(&new_node->rwlock, NULL);
    
    pthread_rwlock_wrlock(&ctx->head.rwlock);
    new_node->next = ctx->head.next;
    ctx->head.next = new_node;
    pthread_rwlock_unlock(&ctx->head.rwlock);
}

void print_list(AppContext* ctx) {
    printf("\n--------------------------------\n");
    Node* prev = &ctx->head;
    
    pthread_rwlock_rdlock(&prev->rwlock);
    
    Node* curr = prev->next;
    int index = 1;
    
    while (curr != NULL) {
        pthread_rwlock_rdlock(&curr->rwlock);
        printf("[%d]: %s\n", index++, curr->str);
        
        Node* next_node = curr->next;
        pthread_rwlock_unlock(&prev->rwlock);
        
        prev = curr;
        curr = next_node;
    }
    pthread_rwlock_unlock(&prev->rwlock);
    printf("--------------------------------\n\n");
}

void bubble_sort(AppContext* ctx) {
    int swapped;
    do {
        sleep(1);
        swapped = 0;
        
        Node* prev = &ctx->head;
        pthread_rwlock_wrlock(&prev->rwlock);
        
        Node* curr = prev->next;
        if (curr == NULL) {
            pthread_rwlock_unlock(&prev->rwlock);
            break;
        }
        pthread_rwlock_wrlock(&curr->rwlock);
        
        while (curr->next != NULL && ctx->is_running) {
            Node* next_node = curr->next;
            pthread_rwlock_wrlock(&next_node->rwlock);
            
            if (strcmp(curr->str, next_node->str) > 0) {
                curr->next = next_node->next;
                next_node->next = curr;
                prev->next = next_node;
                
                swapped = 1;

                Node* old_prev = prev;
                prev = next_node;
                pthread_rwlock_unlock(&old_prev->rwlock);
            } else {
                Node* old_prev = prev;
                prev = curr;
                curr = next_node;
                pthread_rwlock_unlock(&old_prev->rwlock);
            }
        }
        
        if (curr != NULL) pthread_rwlock_unlock(&curr->rwlock);
        if (prev != NULL) pthread_rwlock_unlock(&prev->rwlock);
        
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
    pthread_rwlock_init(&ctx.head.rwlock, NULL);
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
        pthread_rwlock_destroy(&current->rwlock);
        free(current);
        current = next;
    }
    pthread_rwlock_destroy(&ctx.head.rwlock);

    return EXIT_SUCCESS;
}