#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "llist.h"

#define BUFSIZE 1024

int main() {
    llist list;
    llist_init(&list);

    char buffer[BUFSIZE + 1];
    int baddot = 0;

    while (1) {
        if (!fgets(buffer, BUFSIZE + 1, stdin)) {
            break;
        }

        if (buffer[0] == '.' && baddot == 0) {
            break;
        }

        if (baddot) baddot = 0;

        if (buffer[strlen(buffer)-1] != '\n') {
            baddot = 1;
        }

        char *str;
        if (!(str = (char*)malloc(strlen(buffer)+1))) {
            perror("malloc");
            return 1;
        }
        strcpy(str, buffer);
        if (llist_push(&list, str) == -1) {
            perror("malloc in llist push");
            return 1;
        }
    }

    char* str;
    while ((str = llist_pop(&list)) != NULL) {
        if (write(STDOUT_FILENO, str, strlen(str)) == -1) {
            perror("Write error");
            return 1;
        }
        free(str);
    }

    llist_free(&list);

    return 0;
}