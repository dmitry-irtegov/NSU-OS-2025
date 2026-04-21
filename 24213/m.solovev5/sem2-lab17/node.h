typedef struct node_t {
	char *value;
	struct node_t *next;
} node_t;

node_t *node_create();

void node_destroy(node_t *node);

node_t *node_put(node_t *node, char *value);

void node_print(node_t *node);

void node_sort(node_t *node);
