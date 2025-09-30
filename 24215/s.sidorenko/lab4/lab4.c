#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MAX_LEN 4096

typedef struct Node_t {
    struct Node_t* prev;
    struct Node_t* next;
    char* el;
} Node;

int main()
{
    char str[MAX_LEN];
    Node* head = NULL;
    Node* tail = NULL;

    while (fgets(str, MAX_LEN, stdin))
    {
        if (str[0] == '.')
        {
            break;
        }

        size_t len = strlen(str);
        if (len > 0 && str[len - 1] == '\n') 
        {
            str[len - 1] = '\0';
            len--;
        }   

        char* str1 = malloc(len + 1);
        strcpy(str1, str);
        Node* node = malloc(sizeof(Node));

        node->el = str1;
        node->next = NULL;
        node->prev = tail;

        if (!head)
        {
            head = node;
            tail = node;
        }
        else
        {
            tail->next = node;
            tail = node;
        }
    }

    printf("\nYour strings: \n");
    Node *temp;
    for(temp = head; temp; temp = temp->next)
    {
        printf("%s\n", temp->el);
    }

    temp = head;
    while(temp)
    {
        Node *next = temp->next;
        free(temp->el);
        free(temp);
        temp = next;
    }
}