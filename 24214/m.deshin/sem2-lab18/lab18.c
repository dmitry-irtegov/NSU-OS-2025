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
    pthread_mutex_t mutex;
} Node;

static Node head = {
    .text = NULL,
    .next = NULL,
    .mutex = PTHREAD_MUTEX_INITIALIZER
};

static void die(int err, const char *msg) {
    if (err) {
        fprintf(stderr, "%s: %s\n", msg, strerror(err));
        exit(EXIT_FAILURE);
    }
}

static Node *create_node(const char *s) {
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

    node->next = NULL;
    die(pthread_mutex_init(&node->mutex, NULL), "pthread_mutex_init");

    return node;
}

static void insert_front(const char *s) {
    Node *node = create_node(s);

    pthread_mutex_lock(&head.mutex);
    node->next = head.next;
    head.next = node;
    pthread_mutex_unlock(&head.mutex);
}

static void print_list() {
    pthread_mutex_lock(&head.mutex);

    Node *cur = head.next;
    if (cur != NULL) {
        pthread_mutex_lock(&cur->mutex);
    }

    pthread_mutex_unlock(&head.mutex);

    printf("----- list -----\n");

    while (cur != NULL) {
        printf("%s\n", cur->text);

        Node *next = cur->next;
        if (next != NULL) {
            pthread_mutex_lock(&next->mutex);
        }

        pthread_mutex_unlock(&cur->mutex);
        cur = next;
    }

    printf("----------------\n");
}

static void sort_list() {
    int swapped;

    do {
        swapped = 0;

        Node *prev = &head;
        pthread_mutex_lock(&prev->mutex);

        Node *cur = prev->next;
        if (cur != NULL) {
            pthread_mutex_lock(&cur->mutex);
        }

        while (cur != NULL && cur->next != NULL) {
            Node *next = cur->next;
            pthread_mutex_lock(&next->mutex);

            if (strcmp(cur->text, next->text) > 0) {
                cur->next = next->next;
                next->next = cur;
                prev->next = next;

                swapped = 1;

                pthread_mutex_unlock(&prev->mutex);

                prev = next;
            } else {
                pthread_mutex_unlock(&prev->mutex);

                prev = cur;
                cur = next;
            }
        }

        if (cur != NULL) {
            pthread_mutex_unlock(&cur->mutex);
        }

        pthread_mutex_unlock(&prev->mutex);

    } while (swapped);
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
    pthread_mutex_lock(&head.mutex);

    Node *cur = head.next;
    head.next = NULL;

    pthread_mutex_unlock(&head.mutex);

    while (cur != NULL) {
        Node *next = cur->next;
        pthread_mutex_destroy(&cur->mutex);
        free(cur->text);
        free(cur);
        cur = next;
    }
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
    pthread_mutex_destroy(&head.mutex);

    return 0;
}
