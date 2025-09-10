#include "list.h"

#define BUFFER_SIZE 1024

int main()
{
    Node *head = NULL;
    Node *tail = NULL;

    char str_buf[BUFFER_SIZE+1];
    while (1)
    {
        if (fgets(str_buf, BUFFER_SIZE + 1, stdin) == NULL)
        {
            perror("fgets error");
            exit(1);
        }
        if (str_buf[0] == '.')
        {
            break;
        }
        if (head == NULL)
        {
            head = insertNode(head, str_buf);
            tail = head;
            continue;
        }
        tail = insertNode(tail, str_buf);
    }

    printList(head);
    freeList(head);

    return 0;
}