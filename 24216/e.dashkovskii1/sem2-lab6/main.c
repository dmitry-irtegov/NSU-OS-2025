#define _XOPEN_SOURCE 500

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#define MAX_LINES       100
#define MAX_LINE_LEN    1024
#define USLEEP_PER_CHAR 10000

typedef struct {
    char   buf[MAX_LINE_LEN + 2]; 
    size_t write_len;            
} thread_arg;

void handle_error(int en, const char *msg) {
    if (en != 0) {
        errno = en;
        perror(msg);
        exit(EXIT_FAILURE);
    }
}

void *sort_worker(void *arg) {
    thread_arg *targ = (thread_arg *)arg;
    size_t len = targ->write_len - 1;
    usleep((useconds_t)(len * USLEEP_PER_CHAR));
    write(STDOUT_FILENO, targ->buf, targ->write_len);
    return NULL;
}

int main(void) {
    thread_arg args[MAX_LINES];
    pthread_t tids[MAX_LINES];
    int n = 0;

    char line[MAX_LINE_LEN];
    while (n < MAX_LINES && fgets(line, sizeof(line), stdin) != NULL) {
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            len--;
        }
        memcpy(args[n].buf, line, len);
        args[n].buf[len]  = '\n';
        args[n].write_len = len + 1;
        n++;
    }

    for (int i = 0; i < n; i++) {
        handle_error(pthread_create(&tids[i], NULL, sort_worker, &args[i]),
                     "pthread_create");
    }

    for (int i = 0; i < n; i++) {
        handle_error(pthread_join(tids[i], NULL), "pthread_join");
    }

    return EXIT_SUCCESS;
}
