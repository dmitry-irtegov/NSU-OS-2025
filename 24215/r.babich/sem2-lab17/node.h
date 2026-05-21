typedef struct node_t {
	struct node_t *next;
	char *data;
} node_t;

typedef struct singly_linked_list_t {
	node_t *head;
	node_t *tail;
} singly_linked_list_t;

struct node_t* create_node(char *data); 
void initialize(singly_linked_list_t *list); 
void insert_at_beggining(singly_linked_list_t *list, char *data); 
void print(singly_linked_list_t *list); 
void destroy(singly_linked_list_t *list); 
