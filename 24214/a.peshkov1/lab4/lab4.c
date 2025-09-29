#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUF_SIZE 4096

struct Node {
    char *s;
    struct Node *next;
};

int main(void)
{
    char buf[BUF_SIZE];
    struct Node *head = NULL, *tail = NULL;

    printf("Вводите строки. Чтобы завершить ввод, начните ввод с \".\":\n");

    while (fgets(buf, sizeof(buf), stdin) != NULL) {
        size_t len = strlen(buf);
        if (len == 0)
            continue;

        if (buf[len - 1] == '\n') {
            buf[len - 1] = '\0';
            --len;
        }

        if (len > 0 && buf[0] == '.') {
            break;
        }

        // len + 1 для строки + '\0'
        char *copy = malloc(len + 1);

        if (copy == NULL) {
            perror("malloc");

            struct Node *cur = head;
            while (cur) {
                struct Node *next = cur->next;
                free(cur->s);
                free(cur);
                cur = next;
            }
            return EXIT_FAILURE;
        }
        memcpy(copy, buf, len + 1);

        struct Node *node = malloc(sizeof(*node));
        if (node == NULL) {
            perror("malloc");
            free(copy);
            struct Node *cur = head;
            while (cur) {
                struct Node *next = cur->next;
                free(cur->s);
                free(cur);
                cur = next;
            }
            return EXIT_FAILURE;
        }
        node->s = copy;
        node->next = NULL;


        if (tail == NULL) {
            head = tail = node;
        } else {
            tail->next = node;
            tail = node;
        }
    }

    puts("\nСодержимое списка:");
    for (struct Node *cur = head; cur != NULL; cur = cur->next) {
        printf("%s\n", cur->s);
    }

    struct Node *cur = head;
    while (cur) {
        struct Node *next = cur->next;
        free(cur->s);
        free(cur);
        cur = next;
    }

    return EXIT_SUCCESS;
}
