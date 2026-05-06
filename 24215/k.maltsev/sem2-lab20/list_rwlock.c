#define _POSIX_C_SOURCE 200112L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>

#define MAX_CHUNK_LEN 80
#define DEFAULT_SORTER_THREADS 1
#define DEFAULT_SORT_INTERVAL 5

typedef struct Node {
    char *str;
    struct Node *next;
    pthread_rwlock_t lock;
} Node;

typedef struct {
    int sorter_id;
    int interval_sec;
} sorter_arg_t;

static Node head = { NULL, NULL, PTHREAD_RWLOCK_INITIALIZER };

static pthread_rwlock_t state_lock = PTHREAD_RWLOCK_INITIALIZER;
static int stop_requested = 0;

static void die(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

static void die_pthread(const char *msg, int rc) {
    errno = rc;
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

static void rdlock_node(Node *node) {
    int rc = pthread_rwlock_rdlock(&node->lock);
    if (rc != 0) {
        die_pthread("pthread_rwlock_rdlock", rc);
    }
}

static void wrlock_node(Node *node) {
    int rc = pthread_rwlock_wrlock(&node->lock);
    if (rc != 0) {
        die_pthread("pthread_rwlock_wrlock", rc);
    }
}

static void unlock_node(Node *node) {
    int rc = pthread_rwlock_unlock(&node->lock);
    if (rc != 0) {
        die_pthread("pthread_rwlock_unlock", rc);
    }
}

static void rdlock_state(void) {
    int rc = pthread_rwlock_rdlock(&state_lock);
    if (rc != 0) {
        die_pthread("pthread_rwlock_rdlock", rc);
    }
}

static void wrlock_state(void) {
    int rc = pthread_rwlock_wrlock(&state_lock);
    if (rc != 0) {
        die_pthread("pthread_rwlock_wrlock", rc);
    }
}

static void unlock_state(void) {
    int rc = pthread_rwlock_unlock(&state_lock);
    if (rc != 0) {
        die_pthread("pthread_rwlock_unlock", rc);
    }
}

static int is_stop_requested(void) {
    int result;

    rdlock_state();
    result = stop_requested;
    unlock_state();

    return result;
}

static Node *create_node(const char *s) {
    Node *node = (Node *)malloc(sizeof(Node));
    int rc;

    if (node == NULL) {
        die("malloc");
    }

    node->str = xstrdup(s);
    node->next = NULL;

    rc = pthread_rwlock_init(&node->lock, NULL);
    if (rc != 0) {
        free(node->str);
        free(node);
        die_pthread("pthread_rwlock_init", rc);
    }

    return node;
}

static void insert_front(const char *s) {
    Node *node = create_node(s);

    wrlock_node(&head);

    node->next = head.next;
    head.next = node;

    unlock_node(&head);
}

static void print_list(void) {
    Node *cur;
    int index = 0;

    rdlock_node(&head);

    cur = head.next;
    if (cur != NULL) {
        rdlock_node(cur);
    }

    unlock_node(&head);

    printf("----- LIST BEGIN -----\n");

    if (cur == NULL) {
        printf("(empty)\n");
    }

    while (cur != NULL) {
        Node *next = cur->next;

        if (next != NULL) {
            rdlock_node(next);
        }

        printf("%d: %s\n", index, cur->str);
        index++;

        unlock_node(cur);
        cur = next;
    }

    printf("----- LIST END -------\n");
    fflush(stdout);
}

static int bubble_sort_pass(void) {
    int swapped = 0;
    Node *prev;
    Node *cur;

    wrlock_node(&head);

    prev = &head;
    cur = prev->next;

    if (cur == NULL) {
        unlock_node(prev);
        return 0;
    }

    wrlock_node(cur);

    while (cur->next != NULL) {
        Node *next = cur->next;

        wrlock_node(next);

        if (strcmp(cur->str, next->str) > 0) {
            cur->next = next->next;
            next->next = cur;
            prev->next = next;

            swapped = 1;

            unlock_node(prev);

            prev = next;
        } else {
            unlock_node(prev);

            prev = cur;
            cur = next;
        }
    }

    unlock_node(cur);
    unlock_node(prev);

    return swapped;
}

static void bubble_sort_list(void) {
    while (bubble_sort_pass()) {
    }
}

static int wait_for_next_sort_or_stop(int interval_sec) {
    int elapsed = 0;

    while (elapsed < interval_sec) {
        if (is_stop_requested()) {
            return 1;
        }

        sleep(1);
        elapsed++;
    }

    return is_stop_requested();
}

static void *sorter_thread(void *arg) {
    sorter_arg_t *cfg = (sorter_arg_t *)arg;

    (void)cfg->sorter_id;

    for (;;) {
        if (wait_for_next_sort_or_stop(cfg->interval_sec)) {
            break;
        }

        bubble_sort_list();
    }

    return NULL;
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
    size_t chunks;
    size_t i;

    if (len == 0) {
        print_list();
        return;
    }

    chunks = (len + MAX_CHUNK_LEN - 1) / MAX_CHUNK_LEN;

    for (i = chunks; i > 0; --i) {
        size_t start = (i - 1) * MAX_CHUNK_LEN;
        size_t part_len = len - start;
        char part[MAX_CHUNK_LEN + 1];

        if (part_len > MAX_CHUNK_LEN) {
            part_len = MAX_CHUNK_LEN;
        }

        memcpy(part, line + start, part_len);
        part[part_len] = '\0';

        insert_front(part);
    }
}

static void request_stop(void) {
    wrlock_state();
    stop_requested = 1;
    unlock_state();
}

static void free_list(void) {
    Node *cur;

    wrlock_node(&head);

    cur = head.next;
    head.next = NULL;

    unlock_node(&head);

    while (cur != NULL) {
        Node *next = cur->next;
        int rc = pthread_rwlock_destroy(&cur->lock);

        if (rc != 0) {
            die_pthread("pthread_rwlock_destroy", rc);
        }

        free(cur->str);
        free(cur);

        cur = next;
    }
}

static int parse_positive_int(const char *s, const char *name) {
    char *end = NULL;
    long value = strtol(s, &end, 10);

    if (*s == '\0' || *end != '\0' || value <= 0) {
        fprintf(stderr, "Invalid %s: %s\n", name, s);
        exit(EXIT_FAILURE);
    }

    if (value > 1000000L) {
        fprintf(stderr, "%s is too large: %s\n", name, s);
        exit(EXIT_FAILURE);
    }

    return (int)value;
}

int main(int argc, char *argv[]) {
    int sorter_threads = DEFAULT_SORTER_THREADS;
    int interval_sec = DEFAULT_SORT_INTERVAL;
    pthread_t *threads;
    sorter_arg_t *args;
    int i;

    if (argc >= 2) {
        sorter_threads = parse_positive_int(argv[1], "sorter thread count");
    }

    if (argc >= 3) {
        interval_sec = parse_positive_int(argv[2], "sort interval");
    }

    if (argc > 3) {
        fprintf(stderr, "Usage: %s [sorter_threads] [interval_sec]\n", argv[0]);
        return EXIT_FAILURE;
    }

    threads = (pthread_t *)malloc(sizeof(pthread_t) * (size_t)sorter_threads);
    args = (sorter_arg_t *)malloc(sizeof(sorter_arg_t) * (size_t)sorter_threads);

    if (threads == NULL || args == NULL) {
        free(threads);
        free(args);
        die("malloc");
    }

    for (i = 0; i < sorter_threads; ++i) {
        int rc;

        args[i].sorter_id = i;
        args[i].interval_sec = interval_sec;

        rc = pthread_create(&threads[i], NULL, sorter_thread, &args[i]);
        if (rc != 0) {
            request_stop();

            while (--i >= 0) {
                rc = pthread_join(threads[i], NULL);
                if (rc != 0) {
                    die_pthread("pthread_join", rc);
                }
            }

            free(threads);
            free(args);

            die_pthread("pthread_create", rc);
        }
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

    request_stop();

    for (i = 0; i < sorter_threads; ++i) {
        int rc = pthread_join(threads[i], NULL);

        if (rc != 0) {
            die_pthread("pthread_join", rc);
        }
    }

    free(threads);
    free(args);

    free_list();

    {
        int rc = pthread_rwlock_destroy(&head.lock);
        if (rc != 0) {
            die_pthread("pthread_rwlock_destroy", rc);
        }
    }

    {
        int rc = pthread_rwlock_destroy(&state_lock);
        if (rc != 0) {
            die_pthread("pthread_rwlock_destroy", rc);
        }
    }

    return 0;
}