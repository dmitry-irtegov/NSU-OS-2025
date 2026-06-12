#ifndef LIST_H
#define LIST_H

typedef struct List List;

List *init_list();
void push_front(List *list, const char *str);
void print_list(List *list);
void *bubble_sort(void *arg);

#endif
