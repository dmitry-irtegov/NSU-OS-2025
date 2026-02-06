#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <ctype.h>

#define NUM_STEPS 100000000

struct thread_args {
    int start;
    int step;
};

void* thread_func(void* arg) {
    struct thread_args* args = (struct thread_args*)arg;
    
    double* result = malloc(sizeof(double));
    if (!result) {
        perror("malloc failed");
        pthread_exit(NULL);
    }
    *result = 0.0;
    
    for (int i = args->start; i < NUM_STEPS; i += args->step) {
        *result += 1.0/(i*4.0 + 1.0);
        *result -= 1.0/(i*4.0 + 3.0);
    }
    
    pthread_exit(result);
}

int main(int argc, char** argv) {
    if (argc != 2) return 1;
    
    char* arg = argv[1];
    for (int i = 0; arg[i] != '\0'; i++) {
        if (!isdigit(arg[i])) {
            fprintf(stderr, "Error: '%s' It is not number\n", arg);
            return 1;
        }
    }

    int n = atoi(argv[1]);
    if (n <= 0) {
        fprintf(stderr, "The numbers of treads must be positive\n");
        return 1;  
    }
    pthread_t threads[n];
    struct thread_args args[n];
    double pi = 0.0;
    
    for (int i = 0; i < n; i++) {
        args[i].start = i;
        args[i].step = n;
        if (pthread_create(&threads[i], NULL, thread_func, &args[i]) != 0) {
            perror("pthread_create failed");
            return 1;
        }
    }
    
    for (int i = 0; i < n; i++) {
        double* res;
        pthread_join(threads[i], (void**)&res);
        pi += *res;
        free(res);
    }
    
    printf("pi done - %.15g \n", pi * 4.0);
    return 0;
}