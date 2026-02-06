#include <pthread.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h> 
#include <string.h> 

#define handle_error_en(en, msg) do {errno = en; perror(msg); exit(EXIT_FAILURE); } while(0)

void print_all_lines(const char* string) {
    char buffer[256];
    int len = snprintf(buffer, sizeof(buffer), "%s\n", string);
    
    for (int i = 0; i < 10; i++) {
        write(STDOUT_FILENO, buffer, len);
    }
}

void *print_lines(void *arg) {
    (void)arg;
    print_all_lines("thread");
    return NULL;
}

int main() {
    pthread_t tid;
    int result;

    result = pthread_create(&tid, NULL, &print_lines, NULL);

    if (result != 0) {
        handle_error_en(result, "pthread_create");
    }

    result = pthread_join(tid, NULL);
    if (result != 0) {
        handle_error_en(result, "pthread_join");
    }

    print_all_lines("parent");

    exit(EXIT_SUCCESS);
}
