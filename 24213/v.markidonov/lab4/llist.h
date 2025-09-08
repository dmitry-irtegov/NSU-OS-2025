typedef struct listelem {
    struct listelem *next;
    char *str;
} listelem;

typedef struct {
    listelem *first;
    listelem *last;
} llist;

void llist_init(llist* list);

int llist_push(llist* list, char* str);

char* llist_pop(llist* list);

void llist_free(llist* list);