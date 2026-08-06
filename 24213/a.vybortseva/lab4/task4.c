#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MAX_LEN 4096

typedef struct Node {
    struct Node *next;
    char *string;
} Node;

Node *addNode(Node *prevNode, char *value) {
    Node *newNode = (Node *)malloc(sizeof(Node));
    if (newNode == NULL) {
        perror("memory allocation error.");
        return NULL;
    }

    newNode->string = (char *)malloc(strlen(value) + 1);
    if (newNode->string == NULL) {
        perror("memory allocation error.");
        free(newNode);
        return NULL;
    }

    strcpy(newNode->string, value);
    newNode->next = NULL;

    if (prevNode != NULL) {
        prevNode->next = newNode;
    }

    return newNode;
}

void printNodes(Node *head) {
    Node *current = head;

    while (current != NULL) {
        printf("%s", current->string);
        current = current->next;
    }
}

void freeNodes(Node *head) {
    Node *current = head;

    while (current != NULL) {
        Node* temp = current->next;
        free(current->string);
        free(current);
        current = temp;
    }
}

int main() {
    char buffer[MAX_LEN];
    Node *head = NULL;
    Node* current = NULL;

    int ns = 1;

    while (1) {
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            perror("fgets error");
            return -1;
        }

        if (buffer[0] == '.' && ns) {
            break;
        }

        if (buffer[strlen(buffer) - 1] != '\n') {
            ns = 0;
        } else {
            ns = 1;
        }

        if (head == NULL) {
            head = addNode(NULL, buffer);
            if (head == NULL) {
                return -1;
            }
            current = head;
        } else {
            current = addNode(current, buffer);
            if (current == NULL) {
                return -1;
            }
        }
    }

    printNodes(head);
    freeNodes(head);

    return 0;
}
