#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define COPY_CHUNK 65536
#define NAME_FALLBACK 255

typedef struct { 
    char *src, *dst; 
} Job;
typedef struct ThreadNode{ 
    pthread_t tid; struct ThreadNode *next;
} ThreadNode;

void *copy_dir(void *arg);
void *copy_file(void *arg);

void free_job(Job *job) {
    if (job) { free(job->src); free(job->dst); free(job); }
}

Job *make_job(const char *src, const char *dst) {
    Job *job = malloc(sizeof(*job));
    if (!job) return NULL;
    job->src = strdup(src);
    job->dst = strdup(dst);
    if (!job->src || !job->dst) { free_job(job); return NULL; }
    return job;
}

Job *make_child(const char *src_dir, const char *dst_dir, const char *name) {
    size_t src_len = strlen(src_dir) + strlen(name) + 2;
    size_t dst_len = strlen(dst_dir) + strlen(name) + 2;
    Job *job = malloc(sizeof(*job));
    if (!job) return NULL;
    job->src = malloc(src_len);
    job->dst = malloc(dst_len);
    if (!job->src || !job->dst) { free_job(job); return NULL; }
    snprintf(job->src, src_len, "%s/%s", src_dir, name);
    snprintf(job->dst, dst_len, "%s/%s", dst_dir, name);
    return job;
}

int open_retry(const char *path, int flags, mode_t mode) {
    for (;;) {
        int fd = open(path, flags, mode);
        if (fd >= 0) return fd;
        if (errno == EINTR) continue;
        if (errno != EMFILE && errno != ENFILE) return -1;
        sleep(1);
    }
}

DIR *opendir_retry(const char *path) {
    for (;;) {
        DIR *dir = opendir(path);
        if (dir) return dir;
        if (errno == EINTR) continue;
        if (errno != EMFILE && errno != ENFILE) return NULL;
        sleep(1);
    }
}

void *copy_file(void *arg) {
    Job *job = arg;
    struct stat st;
    int in_fd = -1, out_fd = -1;
    char buf[COPY_CHUNK];

    if (stat(job->src, &st) == -1) {
        fprintf(stderr, "cannot stat %s: %s\n", job->src, strerror(errno));
    } else if (S_ISREG(st.st_mode)) {
        in_fd = open_retry(job->src, O_RDONLY, 0);
        if (in_fd == -1) {
            fprintf(stderr, "cannot open %s: %s\n", job->src, strerror(errno));
        } else {
            out_fd = open_retry(job->dst, O_WRONLY | O_CREAT | O_TRUNC, st.st_mode & 0777);
            if (out_fd == -1) {
                fprintf(stderr, "cannot create %s: %s\n", job->dst, strerror(errno));
            } else {
                for (;;) {
                    ssize_t rd = read(in_fd, buf, sizeof(buf));
                    if (rd == 0) break;
                    if (rd < 0) {
                        if (errno == EINTR) continue;
                        fprintf(stderr, "cannot read %s: %s\n", job->src, strerror(errno));
                        break;
                    }
                    for (ssize_t off = 0; off < rd; ) {
                        ssize_t wr = write(out_fd, buf + off, (size_t)(rd - off));
                        if (wr < 0) {
                            if (errno == EINTR) continue;
                            fprintf(stderr, "cannot write %s: %s\n", job->dst, strerror(errno));
                            rd = -1;
                            break;
                        }
                        off += wr;
                    }
                    if (rd < 0) break;
                }
            }
        }
    }
    if (out_fd != -1) close(out_fd);
    if (in_fd != -1) close(in_fd);
    free_job(job);
    return NULL;
}

void *copy_dir(void *arg) {
    Job *job = arg;
    struct stat st;
    DIR *dir = NULL;
    struct dirent *buf = NULL, *entry = NULL;
    ThreadNode *threads = NULL;
    long name_max;
    int rc = 0;

    if (stat(job->src, &st) == -1) {
        fprintf(stderr, "cannot stat %s: %s\n", job->src, strerror(errno));
    } else if (S_ISDIR(st.st_mode)) {
        if (mkdir(job->dst, st.st_mode & 0777) == -1 && errno != EEXIST) {
            fprintf(stderr, "cannot create dir %s: %s\n", job->dst, strerror(errno));
        } else {
            dir = opendir_retry(job->src);
            if (!dir) {
                fprintf(stderr, "cannot open dir %s: %s\n", job->src, strerror(errno));
            } else {
                name_max = pathconf(job->src, _PC_NAME_MAX);
                if (name_max < 0) name_max = NAME_FALLBACK;
                buf = malloc(sizeof(struct dirent) + (size_t)name_max + 1);
                if (buf) {
                    while ((rc = readdir_r(dir, buf, &entry)) == 0 && entry) {
                        struct stat child_st;
                        Job *child;
                        pthread_t tid;
                        ThreadNode *node;
                        int create_err = 0;

                        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")) continue;
                        child = make_child(job->src, job->dst, entry->d_name);
                        if (!child) continue;
                        if (lstat(child->src, &child_st) == -1) {
                            fprintf(stderr, "cannot stat %s: %s\n", child->src, strerror(errno));
                            free_job(child);
                            continue;
                        }
                        if (S_ISDIR(child_st.st_mode)) create_err = pthread_create(&tid, NULL, copy_dir, child);
                        else if (S_ISREG(child_st.st_mode)) create_err = pthread_create(&tid, NULL, copy_file, child);
                        else {
                            free_job(child);
                            continue;
                        }
                        if (create_err != 0) {
                            fprintf(stderr, "cannot create thread for %s: %s\n", child->src, strerror(create_err));
                            free_job(child);
                            continue;
                        }
                        node = malloc(sizeof(*node));
                        if (!node) {
                            pthread_join(tid, NULL);
                            continue;
                        }
                        node->tid = tid;
                        node->next = threads;
                        threads = node;
                    }
                    if (rc != 0) fprintf(stderr, "cannot read dir %s: %s\n", job->src, strerror(rc));
                }
            }
        }
    }
    free(buf);
    while (threads) {
        ThreadNode *next = threads->next;
        pthread_join(threads->tid, NULL);
        free(threads);
        threads = next;
    }
    if (dir) closedir(dir);
    free_job(job);
    return NULL;
}

int main(int argc, char **argv) {
    Job *root;
    if (argc != 3) {
        fprintf(stderr, "usage: %s <source_dir> <target_dir>\n", argv[0]);
        return 1;
    }
    root = make_job(argv[1], argv[2]);
    if (!root) {
        fprintf(stderr, "memory allocation failed\n");
        return 1;
    }
    copy_dir(root);
    return 0;
}
