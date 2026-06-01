#define _POSIX_C_SOURCE 200809L

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_LINE 80
#define SORT_INTERVAL 5

typedef struct Node {
    char *text;
    struct Node *next;
} Node;

static Node *head = NULL;
static pthread_mutex_t list_mutex = PTHREAD_MUTEX_INITIALIZER;

static void die(int err, const char *msg) {
    if (err) {
        fprintf(stderr, "%s: %s\n", msg, strerror(err));
        exit(EXIT_FAILURE);
    }
}

static void insert_front(const char *s) {
    Node *node = malloc(sizeof(*node));
    if (!node) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    node->text = strdup(s);
    if (!node->text) {
        perror("strdup");
        exit(EXIT_FAILURE);
    }

    pthread_mutex_lock(&list_mutex);
    node->next = head;
    head = node;
    pthread_mutex_unlock(&list_mutex);
}

static void print_list() {
    pthread_mutex_lock(&list_mutex);

    printf("----- list -----\n");
    for (Node *cur = head; cur != NULL; cur = cur->next) {
        printf("%s\n", cur->text);
    }
    printf("----------------\n");

    pthread_mutex_unlock(&list_mutex);
}

static void sort_list() {
    pthread_mutex_lock(&list_mutex);

    int swapped;
    do {
        swapped = 0;
        Node **pp = &head;

        while (*pp != NULL && (*pp)->next != NULL) {
            Node *a = *pp;
            Node *b = a->next;

            if (strcmp(a->text, b->text) > 0) {
                a->next = b->next;
                b->next = a;
                *pp = b;
                swapped = 1;
            }

            pp = &((*pp)->next);
        }
    } while (swapped);

    pthread_mutex_unlock(&list_mutex);
}

static void *sort_thread(void *arg) {
    (void)arg;
    
    while (1) {
        sleep(SORT_INTERVAL);
        sort_list();
    }
    
    return NULL;
}

static void destroy_list() {
    pthread_mutex_lock(&list_mutex);

    Node *cur = head;
    while (cur != NULL) {
        Node *next = cur->next;
        free(cur->text);
        free(cur);
        cur = next;
    }

    head = NULL;

    pthread_mutex_unlock(&list_mutex);
}

int main() {
    pthread_t tid;
    die(pthread_create(&tid, NULL, sort_thread, NULL), "pthread_create");

    char buf[MAX_LINE + 2];

    while (fgets(buf, sizeof(buf), stdin) != NULL) {
        buf[strcspn(buf, "\n")] = '\0';

        if (buf[0] == '\0') {
            print_list();
        } else {
            insert_front(buf);
        }
    }

    pthread_cancel(tid);
    pthread_join(tid, NULL);

    destroy_list();
    pthread_mutex_destroy(&list_mutex);

    return 0;
}
