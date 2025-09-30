#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "queue.h"

#define BUFSIZE 512

int main() {
    queue q;
    queue_init(&q);

    char buffer[BUFSIZE + 1];
    int baddot = 0;

    while (1) {
        if (!fgets(buffer, BUFSIZE + 1, stdin)) {
            exit(EXIT_FAILURE);
        }

        if (buffer[0] == '.' && baddot == 0) {
            break;
        }

        if (buffer[strlen(buffer)-1] == '\n') {
            baddot = 0;
        } else {
            baddot = 1;
        }

        char *str;
        if (!(str = (char*)malloc(strlen(buffer) + 1))) {
            perror("malloc");
            exit(EXIT_FAILURE);
        }
        strcpy(str, buffer);
        if (enqueue(&q, str) == -1) {
            perror("malloc in enqueue");
            exit(EXIT_FAILURE);
        }
    }

    char* str;
    while ((str = dequeue(&q)) != NULL) {
        if (printf("%s", str) < 0) {
            perror("printf error");
            exit(EXIT_FAILURE);
        }
        free(str);
    }

    queue_free(&q);

    return 0;
}
