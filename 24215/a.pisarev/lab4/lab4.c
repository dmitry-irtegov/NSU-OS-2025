#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_SIZE 2000

typedef struct Node {
    char *string;
    struct Node *next;
} Node;

int main() {
    Node *head = NULL;
    Node *tail = NULL;

    char buf[MAX_SIZE];

    while (1) {
        if (fgets(buf, MAX_SIZE, stdin) == NULL) {
            perror("fgets");
            return 1;
        }

        if (buf[0] == '.') {
            break;
        }
      
        Node *newNode = malloc(sizeof(Node));
        if (!newNode) {
            perror("malloc");
            return 1;
        }

        int len = strlen(buf);
        newNode->string = malloc(len + 1);
        if (!newNode->string) {
            perror("malloc");
            return 1;
        }
        strcpy(newNode->string, buf);
        newNode->next = NULL;

        if (head == NULL) {
            head = newNode;
            tail = newNode;
        } 
        else {
            tail->next = newNode;
            tail = newNode;
        }
    }


    Node *p = head;
    while (p != NULL) {
        printf("%s", p->string);

        Node *tmp = p;
        p = p->next;

        free(tmp->string);
        free(tmp);
    }

    return 0;
}
