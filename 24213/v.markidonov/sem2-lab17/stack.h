#include <pthread.h>

typedef struct stack {
    struct stack_el* top;
    pthread_mutex_t* mutex;
} stack;

typedef struct stack_el {
    struct stack_el* next;
    char* str;
} stack_el;

void stack_init(stack* st, pthread_mutex_t* mutex);

void stack_push(stack* st, char* str, int len);

void stack_print(stack* st);

void stack_sort(stack* st);
