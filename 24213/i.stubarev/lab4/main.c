#include "list.h"

#define BUFFER_SIZE 1024

int main()
{
    Node *head = NULL;
    Node *tail = NULL;

    char str_buf[BUFFER_SIZE + 1];
    int new_str = 1;

    while (fgets(str_buf, BUFFER_SIZE + 1, stdin) != NULL)
    {
        if (str_buf[0] == '.' && new_str)
        {
            break;
        }

        if (str_buf[strlen(str_buf) - 1] == '\n')
        {
            new_str = 1;
        }
        else
        {
            new_str = 0;
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
