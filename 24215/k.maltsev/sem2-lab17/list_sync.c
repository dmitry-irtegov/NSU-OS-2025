#define _POSIX_C_SOURCE 200112L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>

#define MAX_CHUNK_LEN 80

typedef struct Node {
    char *str;
    struct Node *next;
} Node;

static Node *head = NULL;
static pthread_mutex_t list_mutex = PTHREAD_MUTEX_INITIALIZER;
static int finished = 0;

static void die(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

static char *xstrdup(const char *s) {
    size_t len = strlen(s);
    char *copy = (char *)malloc(len + 1);
    if (copy == NULL) {
        die("malloc");
    }
    memcpy(copy, s, len + 1);
    return copy;
}

static void push_front(const char *s) {
    Node *node = (Node *)malloc(sizeof(Node));
    if (node == NULL) {
        die("malloc");
    }

    node->str = xstrdup(s);

    if (pthread_mutex_lock(&list_mutex) != 0) {
        die("pthread_mutex_lock");
    }

    node->next = head;
    head = node;

    if (pthread_mutex_unlock(&list_mutex) != 0) {
        die("pthread_mutex_unlock");
    }
}

static void print_list(void) {
    if (pthread_mutex_lock(&list_mutex) != 0) {
        die("pthread_mutex_lock");
    }

    Node *cur = head;
    int index = 0;

    printf("----- LIST BEGIN -----\n");
    if (cur == NULL) {
        printf("(empty)\n");
    } else {
        while (cur != NULL) {
            printf("%d: %s\n", index, cur->str);
            cur = cur->next;
            index++;
        }
    }
    printf("----- LIST END -------\n");
    fflush(stdout);

    if (pthread_mutex_unlock(&list_mutex) != 0) {
        die("pthread_mutex_unlock");
    }
}

static void bubble_sort_list(void) {
    int swapped;

    if (pthread_mutex_lock(&list_mutex) != 0) {
        die("pthread_mutex_lock");
    }

    if (head == NULL || head->next == NULL) {
        if (pthread_mutex_unlock(&list_mutex) != 0) {
            die("pthread_mutex_unlock");
        }
        return;
    }

    do {
        Node *cur = head;
        swapped = 0;

        while (cur != NULL && cur->next != NULL) {
            if (strcmp(cur->str, cur->next->str) > 0) {
                char *tmp = cur->str;
                cur->str = cur->next->str;
                cur->next->str = tmp;
                swapped = 1;
            }
            cur = cur->next;
        }
    } while (swapped);

    if (pthread_mutex_unlock(&list_mutex) != 0) {
        die("pthread_mutex_unlock");
    }
}

static void free_list(void) {
    if (pthread_mutex_lock(&list_mutex) != 0) {
        die("pthread_mutex_lock");
    }

    Node *cur = head;
    head = NULL;

    while (cur != NULL) {
        Node *next = cur->next;
        free(cur->str);
        free(cur);
        cur = next;
    }

    if (pthread_mutex_unlock(&list_mutex) != 0) {
        die("pthread_mutex_unlock");
    }
}

static int read_line_dynamic(char **buf_out) {
    size_t cap = 128;
    size_t len = 0;
    int ch;
    char *buf = (char *)malloc(cap);

    if (buf == NULL) {
        die("malloc");
    }

    while ((ch = getchar()) != EOF) {
        if (ch == '\n') {
            break;
        }

        if (len + 1 >= cap) {
            size_t new_cap = cap * 2;
            char *tmp = (char *)realloc(buf, new_cap);
            if (tmp == NULL) {
                free(buf);
                die("realloc");
            }
            buf = tmp;
            cap = new_cap;
        }

        buf[len++] = (char)ch;
    }

    if (ch == EOF && len == 0) {
        free(buf);
        return 0;
    }

    buf[len] = '\0';
    *buf_out = buf;
    return 1;
}

static void insert_line_chunks(const char *line) {
    size_t len = strlen(line);

    if (len == 0) {
        print_list();
        return;
    }

    size_t chunks = (len + MAX_CHUNK_LEN - 1) / MAX_CHUNK_LEN;
    size_t i;

    for (i = chunks; i > 0; --i) {
        size_t start = (i - 1) * MAX_CHUNK_LEN;
        size_t part_len = len - start;

        if (part_len > MAX_CHUNK_LEN) {
            part_len = MAX_CHUNK_LEN;
        }

        char part[MAX_CHUNK_LEN + 1];
        memcpy(part, line + start, part_len);
        part[part_len] = '\0';

        push_front(part);
    }
}

static void *sorter_thread(void *arg) {
    (void)arg;

    for (;;) {
        sleep(5);

        if (pthread_mutex_lock(&list_mutex) != 0) {
            die("pthread_mutex_lock");
        }

        if (finished) {
            if (pthread_mutex_unlock(&list_mutex) != 0) {
                die("pthread_mutex_unlock");
            }
            break;
        }

        if (pthread_mutex_unlock(&list_mutex) != 0) {
            die("pthread_mutex_unlock");
        }

        bubble_sort_list();
    }

    return NULL;
}

int main(void) {
    pthread_t tid;

    if (pthread_create(&tid, NULL, sorter_thread, NULL) != 0) {
        die("pthread_create");
    }

    for (;;) {
        char *line = NULL;
        int rc = read_line_dynamic(&line);

        if (rc == 0) {
            break;
        }

        insert_line_chunks(line);
        free(line);
    }

    if (pthread_mutex_lock(&list_mutex) != 0) {
        die("pthread_mutex_lock");
    }
    finished = 1;
    if (pthread_mutex_unlock(&list_mutex) != 0) {
        die("pthread_mutex_unlock");
    }

    if (pthread_join(tid, NULL) != 0) {
        die("pthread_join");
    }

    free_list();
    if (pthread_mutex_destroy(&list_mutex) != 0) {
        die("pthread_mutex_destroy");
    }

    return 0;
}