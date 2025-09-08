typedef struct qelem {
    struct qelem *next;
    char *str;
} qelem;

typedef struct {
    qelem *first;
    qelem *last;
} queue;

void queue_init(queue* q);

int enqueue(queue* q, char* str);

char* dequeue(queue* q);

void queue_free(queue* q);
