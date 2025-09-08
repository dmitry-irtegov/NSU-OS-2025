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

    while (fgets(buffer, BUFSIZE + 1, stdin)) {
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
            return 1;
        }
        strcpy(str, buffer);
        if (enqueue(&q, str) == -1) {
            perror("malloc in enqueue");
            return 1;
        }
    }

    char* str;
    while ((str = dequeue(&q)) != NULL) {
        printf("%s", str);
        free(str);
    }

    queue_free(&q);

    return 0;
}
