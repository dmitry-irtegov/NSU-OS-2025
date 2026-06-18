#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#define MAX_LINE_LEN 4096
#define CHUNK_SIZE 80

typedef struct Node {
    char str[CHUNK_SIZE + 1];
    struct Node* next;
} Node;


typedef struct {
    Node* head;
    pthread_mutex_t mutex;
    int is_running;
} AppContext;

void push_front(AppContext* ctx, const char* str) {
    Node* new_node = (Node*)malloc(sizeof(Node));
    if (!new_node) {
        perror("Ошибка выделения памяти");
        exit(EXIT_FAILURE);
    }
    strncpy(new_node->str, str, CHUNK_SIZE);
    new_node->str[CHUNK_SIZE] = '\0';
    
    new_node->next = ctx->head;
    ctx->head = new_node;
}

void print_list(AppContext* ctx) {
    printf("\n--------------------------------\n");
    Node* current = ctx->head;
    int index = 1;
    while (current != NULL) {
        printf("[%d]: %s\n", index++, current->str);
        current = current->next;
    }
    printf("--------------------------------\n\n");
}

void bubble_sort(AppContext* ctx) {
    if (ctx->head == NULL || ctx->head->next == NULL) return;

    int swapped;
    Node* ptr1;
    Node* lptr = NULL;

    do {
        swapped = 0;
        ptr1 = ctx->head;

        while (ptr1->next != lptr) {
            if (strcmp(ptr1->str, ptr1->next->str) > 0) {
                char temp[CHUNK_SIZE + 1];
                strcpy(temp, ptr1->str);
                strcpy(ptr1->str, ptr1->next->str);
                strcpy(ptr1->next->str, temp);
                swapped = 1;
            }
            ptr1 = ptr1->next;
        }
        lptr = ptr1;
    } while (swapped);
}

void* sort_thread_func(void* arg) {
    AppContext* ctx = (AppContext*)arg;
    while (1) {
        sleep(5);
        pthread_mutex_lock(&ctx->mutex);

        if (!ctx->is_running) {
            pthread_mutex_unlock(&ctx->mutex);
            break;
        }

        bubble_sort(ctx);
        pthread_mutex_unlock(&ctx->mutex);
    }
    return NULL;
}
void free_list(AppContext* ctx) {
    Node* current = ctx->head;
    while (current != NULL) {
        Node* next = current->next;
        free(current);
        current = next;
    }
}

int main() {
    pthread_t sort_thread;
    char buffer[MAX_LINE_LEN];

    AppContext ctx = {
        .head = NULL,
        .mutex = PTHREAD_MUTEX_INITIALIZER,
        .is_running = 1
    };

    if (pthread_create(&sort_thread, NULL, sort_thread_func, &ctx) != 0) {
        perror("Ошибка создания потока");
        return EXIT_FAILURE;
    }


    while (fgets(buffer, sizeof(buffer), stdin) != NULL) {
        size_t len = strlen(buffer);
        
        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
            len--;
        }

        pthread_mutex_lock(&ctx.mutex);

        if (len == 0) {
            print_list(&ctx);
        } else {
            for (size_t i = 0; i < len; i += CHUNK_SIZE) {
                char chunk[CHUNK_SIZE + 1] = {0};
                strncpy(chunk, buffer + i, CHUNK_SIZE);
                push_front(&ctx, chunk);
            }
        }

        pthread_mutex_unlock(&ctx.mutex);
    }

    ctx.is_running = 0;
    
    pthread_join(sort_thread, NULL); 
    
    free_list(&ctx);
    pthread_mutex_destroy(&ctx.mutex);

    return EXIT_SUCCESS;
}