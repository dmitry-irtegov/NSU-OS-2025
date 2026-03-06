#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#define MAX_LINES 100

typedef struct {
    char* str;
    int length;
} line_t;

void* sleep_and_print(void* arg) {
    line_t* data = (line_t*)arg;

    usleep(data->length * 100000);
    char* buf = malloc(sizeof(char) * (data->length + 1));
    sprintf(buf, "%s\n", data->str);
    write(1, buf, data->length + 1);
    free(buf);

    free(data->str);
    free(data);

    return NULL;
}

int main() {
    char* lines[MAX_LINES];
    int line_count = 0;
    pthread_t threads[MAX_LINES];
    char* buffer = NULL;
    size_t buffer_size = 0;
    ssize_t read_size;

    while ((read_size = getline(&buffer, &buffer_size, stdin)) != -1 && line_count < MAX_LINES) {
        if (read_size > 0 && buffer[read_size - 1] == '\n') {
            buffer[read_size - 1] = '\0';
            read_size--;
        }

        lines[line_count] = malloc(read_size + 1);
        if (lines[line_count] == NULL) {
            perror("malloc failed");
            exit(EXIT_FAILURE);
        }
        strcpy(lines[line_count], buffer);
        line_count++;
    }
    if (buffer) {
        free(buffer);
    }

    for (int i = 0; i < line_count; i++) {
        line_t* data = malloc(sizeof(line_t));
        if (data == NULL) {
            perror("malloc failed");
            exit(EXIT_FAILURE);
        }

        data->str = lines[i];
        data->length = strlen(lines[i]);

        if (pthread_create(&threads[i], NULL, sleep_and_print, data) != 0) {
            perror("pthread_create failed");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < line_count; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("pthread_join failed");
        }
    }

    return 0;
}