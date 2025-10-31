#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Node {
    char* string;
    struct Node* next;
};

struct Node* makeNode(char* str) {
    struct Node* newnode = (struct Node*)malloc(sizeof(struct Node));
    if (newnode == 0) {
        printf("Node memory allocation error");
        return NULL;
    }
    newnode->string = (char*)malloc(strlen(str) + 1);
    if (newnode->string == NULL) {
        printf("Row memory allocation error");
        return NULL;
    }
    strcpy(newnode->string, str);
    newnode->next = NULL;
    return newnode;
}

void appendNewNode(struct Node **head, char *str) {
    struct Node* newnode = makeNode(str);
    if (newnode == NULL) {
        return;
    }
    if (*head == NULL) {
        *head = newnode;
    }
    else {
        struct Node *findend = *head;
        while (findend->next != NULL) {
            findend = findend->next;
        }
        findend->next = newnode;
    }
}

int main(){
    char buffer[2048];
    struct Node* head = NULL;
    while (1) {
        if (fgets(buffer, 2048, stdin) == NULL) {
            printf("Input error");
            break;
        }
        if (buffer[0] == '.') {
            break;
        }
        appendNewNode(&head, buffer);
    }
    struct Node *forprint = head;
    printf("\n");
    while (forprint != NULL) {
        printf("%s", forprint->string);
        forprint = forprint->next;
    }
    struct Node* temp;
    while (head != NULL) {
        temp = head;
        head = head->next;
        free(temp->string);
        free(temp);
    }
    return(0);
}
