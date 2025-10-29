#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef struct Node{
    char *buffer;
    struct Node *next;
} Node;


void push_back(Node** head, Node** tail, char* buffer){
    size_t str_size = strlen(buffer);

    Node *new = malloc(sizeof(Node));
    new->buffer = malloc(sizeof(char) * (str_size + 1));
    new->next = NULL;

    memcpy(new->buffer, buffer, str_size + 1);

    if (*tail){
        (*tail)->next = new;
    }
    else{
        *head = new;
        *tail = new;
    }

    *tail = new;
}


int main(){
    int MAX_LEN = 4096;

    Node *head = NULL, *tail = NULL;
    char *buffer = malloc(sizeof(char) * MAX_LEN);

    while (fgets(buffer, MAX_LEN, stdin)){
        if(buffer[0] == '.'){
            break;
        }

        buffer[strcspn(buffer, "\n")] = '\0';
        push_back(&head, &tail, buffer);
    }

    for (Node *cur = head; cur != NULL; cur = cur->next){
        puts(cur->buffer);
    }

    free(buffer);
    Node *cur = head;
    while(cur){
        Node *next = cur->next;
        free(cur->buffer);
        free(cur);

        cur = next;
    }

    return 0;
}
