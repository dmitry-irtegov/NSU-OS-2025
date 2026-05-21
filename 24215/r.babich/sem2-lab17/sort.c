#include "sort.h"
#include <string.h>

void bubble_sort(singly_linked_list_t *list) {
	if (!list->head) {
		return;
	}
	int swapped;
  node_t *ptr;
	node_t *last = NULL;
	do {
  	swapped = 0;
    ptr = list->head;

    while (ptr->next != last) {
    	if (ptr->data && ptr->next->data && strcmp(ptr->data, ptr->next->data) > 0) {
      	char *temp_data = ptr->data;
        ptr->data = ptr->next->data;
        ptr->next->data = temp_data;
        swapped = 1;
      }
      ptr = ptr->next;
    }
    last = ptr;
  } while (swapped);
}
