#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#define MAX_LINES 100
#define MAX_LINE_LEN 4096
#define USLEEP_PER_CHAR 10000

static void *thread_func(void *arg) {
    char *line = (char *)arg;
    usleep(strlen(line) * USLEEP_PER_CHAR);
    puts(line);
    free(line);
    return NULL;
}

int main(void) {
    pthread_t threads[MAX_LINES];
    int count = 0;
    char buf[MAX_LINE_LEN];

    while (count < MAX_LINES && fgets(buf, sizeof(buf), stdin) != NULL) {
        size_t len = strlen(buf);
        if (len > 0 && buf[len - 1] == '\n') {
            buf[--len] = '\0';
        }

        char *line = strdup(buf);
        if (line == NULL) {
            perror("strdup");
            return 1;
        }

        if (pthread_create(&threads[count], NULL, thread_func, line) != 0) {
            perror("pthread_create");
            free(line);
            return 1;
        }
        count++;
    }

    for (int i = 0; i < count; i++) {
        pthread_join(threads[i], NULL);
    }

    return 0;
}
