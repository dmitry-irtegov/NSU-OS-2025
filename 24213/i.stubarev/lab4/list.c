#include "list.h"

Node *createNode(char *data)
{
    Node *newNode = (Node *)malloc(sizeof(Node));
    if (newNode == NULL)
    {
        perror("Memory allocation for node failed");
        exit(1);
    }
    newNode->data = (char *)malloc(strlen(data) + 1);
    if (newNode->data == NULL)
    {
        perror("Memory allocation for data failed");
        exit(1);
    }
    strcpy(newNode->data, data);
    newNode->next = NULL;
    return newNode;
}

Node *insertNode(Node *tail, char *data)
{
    Node *newNode = createNode(data);
    if (newNode == NULL)
    {
        perror("Memory allocation for data failed");
        exit(1);
    }
    if (tail == NULL)
    {
        return newNode;
    }
    else
    {
        tail->next = newNode;
        return newNode;
    }
}

void printList(Node *head)
{
    Node *cur = head;
    while (cur != NULL)
    {
        printf("%s", cur->data);
        cur = cur->next;
    }
}

void freeList(Node *head)
{
    Node *cur = head;
    while (cur != NULL)
    {
        Node *next = cur->next;
        free(cur->data);
        free(cur);
        cur = next;
    }
}
