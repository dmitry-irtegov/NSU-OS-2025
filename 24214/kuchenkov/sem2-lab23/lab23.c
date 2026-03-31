#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

typedef struct Node {
    char *str;
    struct Node *next;
} Node;

Node *head = NULL;
Node *tail = NULL;
pthread_mutex_t list_mutex = PTHREAD_MUTEX_INITIALIZER;

void* thread_function(void* arg) {
	char *a = (char*)arg;
	usleep(strlen(a) * 100000);
    
    Node *new_node = malloc(sizeof(Node));
    new_node->str = a;
    new_node->next = NULL;

    pthread_mutex_lock(&list_mutex);

    if (head == NULL) {
        head = new_node;
        tail = new_node;
    } else {
        tail->next = new_node;
        tail = new_node;
    }

    pthread_mutex_unlock(&list_mutex);
	return NULL;
}

int main() {
	int n = 0;
	pthread_t thread[100];
	char buffer[1024];

	while (n < 100 && fgets(buffer, sizeof(buffer), stdin) != NULL) {
		char *copy = strdup(buffer);
		int code = pthread_create(&thread[n], NULL, thread_function, copy);
		if (code != 0) {
			fprintf(stderr, "Failed to create a thread");
			return 1;
		}
		n++;
	}
    
	for (int i = 0; i < n; i++) {
		int code = pthread_join(thread[i], NULL);
		if (code != 0) {
			fprintf(stderr, "Failed to join a thread");
			return 1;
		}
	}

    Node *current = head;
    while (current != NULL) {
        printf("%s", current->str);
        Node *temp = current;
        current = current->next;
        free(temp->str);
        free(temp);
    }

    pthread_mutex_destroy(&list_mutex);
	return 0;
}