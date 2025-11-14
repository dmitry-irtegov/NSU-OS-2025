#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MAX_LEN 1024

typedef struct Node
{
    struct Node *next;
    char *string;
} Node;

Node *addNode(Node *prevNode, char *value)
{
    Node *newNode = (Node *)malloc(sizeof(Node));
    if (newNode == NULL)
    {
        perror("Memory allocation error.");
        return NULL;
    }

    newNode->string = (char *)malloc(strlen(value) + 1);
    if (newNode->string == NULL)
    {
        perror("Memory allocation error.");
        free(newNode);
        return NULL;
    }

    strcpy(newNode->string, value);
    newNode->next = NULL;

    if (prevNode != NULL)
    {
        prevNode->next = newNode;
    }

    return newNode;
}

void printNodes(Node *head)
{
    Node *current = head;

    while (current != NULL)
    {
        printf("%s", current->string);
        current = current->next;
    }
}

void freeNodes(Node *head)
{
    Node *current = head;

    while (current != NULL)
    {
        Node* temp = current->next;
        free(current->string);
        free(current);
        current = temp;
    }
}

int main()
{
    char buffer[MAX_LEN];
    Node *head = NULL;
    Node* current = NULL;

    while (1)
    {
        if (fgets(buffer, sizeof(buffer), stdin) == NULL)
        {
            perror("Reading the string with error");
            return 1;
        }

        if (buffer[0] == '.')
        {
            break;
        }

        if (head == NULL)
        {
            head = addNode(NULL, buffer);     
            current = head;
        } else {
            current = addNode(current, buffer);
        }   
    }

    printNodes(head);
    freeNodes(head);

    return 0;
}
