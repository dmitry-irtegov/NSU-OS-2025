#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>

uint32_t x = 0x8846;
int is_found = 0;
int global_n = 0;
int global_thread_count = 0;

void print_bits(uint32_t value) {
    for (int i = 31; i >= 0; i--) {
        printf("%d", (value >> i) & 1);
    }
}

uint32_t toy_hash(uint32_t x, uint32_t salt) {
    uint32_t h = x ^ salt;
    h *= 0x7feb352d;
    h ^= h >> 15;
    h *= 0x846ca68b;
    h ^= h >> 16;
    return h;
}

void* check_salt(void* arg) {
    int n = (int)(intptr_t)arg;
    uint32_t salt = (uint32_t)n;
    uint64_t max_salt = ((uint64_t)UINT32_MAX + 1) / global_thread_count;

    for (uint64_t i = 0x0; i < max_salt; i++) {
        if (is_found) {
            break;
        }

        uint32_t hash = toy_hash(x, salt);
        if (hash >> (32 - global_n) == 0) {
            is_found = 1;

            uint32_t* result = malloc(2 * sizeof(uint32_t));
            result[0] = salt;
            result[1] = hash;
            pthread_exit(result);
        }
        salt += global_thread_count;
    }

    return NULL;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <num_threads>\n", argv[0]);
        return 1;
    }
    global_thread_count = atoi(argv[1]);

    pthread_t* threads = malloc(sizeof(pthread_t) * global_thread_count);
    
    int code;

    for (int i = 1; i <= 32; i++) {
        global_n = i;
        is_found = 0;

        for (int n = 0; n < global_thread_count; n++) {
            code = pthread_create(&threads[n], NULL, check_salt, (void*)(intptr_t)n);
            if (code != 0) {
                fprintf(stderr, "Error creating thread %d.\n", n);
                return 1;
            }
        }

        int printed = 0;
        
        for (int n = 0; n < global_thread_count; n++) {
            uint32_t* result;
            code = pthread_join(threads[n], (void**)&result);
            if (code != 0) {
                fprintf(stderr, "Error joining thread %d.\n", n);
                return 1;
            }
            if (result != NULL && !printed) {                
                printf("Zeros: %2d | ", i);
                printf("Hash: ");
                print_bits(result[1]);
                printf(" | Salt: ");
                print_bits(result[0]);
                printf("\n");

                printed = 1;
            }
            free(result);
        }

        if (!printed) {
            printf("Zeros: %2d | No solution found.\n", i);
            break;
        }
    }

    free(threads);
    return 0;
}