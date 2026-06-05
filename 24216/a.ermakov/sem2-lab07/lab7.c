#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define COPY_BUF_SIZE (64 * 1024)

void error_log(const char *operation, const char *context) {
    fprintf(stderr, "ERROR: %s", operation);
    if (context) fprintf(stderr, " (%s)", context);
    if (errno != 0) fprintf(stderr, ": %s", strerror(errno));
    fprintf(stderr, "\n");
    fflush(stderr);
}

void fatal_error(const char *operation, const char *context) {
    error_log(operation, context);
    exit(EXIT_FAILURE);
}

struct task {
    char *src;
    char *dst;
    int is_dir;
};

char *join_paths(const char *dir, const char *name) {
    size_t len = strlen(dir) + strlen(name) + 2;
    char *path = malloc(len);
    if (path) snprintf(path, len, "%s/%s", dir, name);
    return path;
}

void wait_for_fd() { 
    sleep(1); 
}

void copy_file(const char *src, const char *dst) {
    struct stat st;
    if (stat(src, &st) != 0) {
        error_log("stat failed", src);
        return;
    }

    int src_fd, dst_fd;

    while ((src_fd = open(src, O_RDONLY)) < 0) {
        if (errno == EMFILE) wait_for_fd();
        else if (errno != EINTR) {
            error_log("failed to open source file", src);
            return;
        }
    }

    while ((dst_fd = open(dst, O_WRONLY | O_CREAT | O_TRUNC, st.st_mode & 0777)) < 0) {
        if (errno == EMFILE) wait_for_fd();
        else if (errno != EINTR) {
            error_log("failed to create destination file", dst);
            close(src_fd);
            return;
        }
    }

    char buf[COPY_BUF_SIZE];
    ssize_t r, w, off;
    
    while ((r = read(src_fd, buf, sizeof(buf))) != 0) {
        if (r < 0) {
            if (errno == EINTR) continue;
            error_log("read failed", src);
            break;
        }
        off = 0;
        while (off < r) {
            w = write(dst_fd, buf + off, r - off);
            if (w < 0) {
                if (errno == EINTR) continue;
                error_log("write failed", dst);
                close(src_fd);
                close(dst_fd);
                return;
            }
            off += w;
        }
    }

    close(src_fd); 
    close(dst_fd);
}

void traverse_dir(const char *src_dir, const char *dst_dir);

void *worker(void *arg) {
    struct task *t = (struct task *)arg;
    if (t->is_dir) traverse_dir(t->src, t->dst);
    else copy_file(t->src, t->dst);
    
    free(t->src); 
    free(t->dst); 
    free(t);
    return NULL;
}

void traverse_dir(const char *src_dir, const char *dst_dir) {
    DIR *dirp;
    while (!(dirp = opendir(src_dir))) {
        if (errno == EMFILE) wait_for_fd();
        else {
            error_log("failed to open directory", src_dir);
            return;
        }
    }

    long name_max = pathconf(src_dir, _PC_NAME_MAX);
    if (name_max == -1) name_max = 255;
    struct dirent *entry = malloc(sizeof(struct dirent) + name_max + 1);
    if (!entry) {
        error_log("memory allocation failed", "dirent");
        closedir(dirp);
        return;
    }

    struct dirent *res;
    pthread_t *threads = NULL;
    size_t count = 0, cap = 0;

    while (1) {
        int rc = readdir_r(dirp, entry, &res);
        if (rc == EMFILE) {
            wait_for_fd();
            continue;
        }
        if (rc != 0) {
            error_log("readdir_r failed", src_dir);
            break;
        }
        if (!res) break;

        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;

        char *src_path = join_paths(src_dir, entry->d_name);
        char *dst_path = join_paths(dst_dir, entry->d_name);
        struct stat st;

        if (!src_path || !dst_path) {
            error_log("path allocation failed", entry->d_name);
            free(src_path);
            free(dst_path);
            continue;
        }

        if (lstat(src_path, &st) != 0) {
            error_log("lstat failed", src_path);
            free(src_path);
            free(dst_path);
            continue;
        }

        if (S_ISLNK(st.st_mode)) {
            free(src_path);
            free(dst_path);
            continue;
        }

        int is_dir = S_ISDIR(st.st_mode);
        if (is_dir) {
            if (mkdir(dst_path, st.st_mode & 0777) != 0 && errno != EEXIST) {
                error_log("mkdir failed", dst_path);
                free(src_path);
                free(dst_path);
                continue;
            }
        } else if (!S_ISREG(st.st_mode)) {
            free(src_path);
            free(dst_path);
            continue;
        }

        struct task *t = malloc(sizeof(*t));
        if (!t) {
            error_log("memory allocation failed", "task");
            free(src_path);
            free(dst_path);
            continue;
        }
        t->src = src_path;
        t->dst = dst_path;
        t->is_dir = is_dir;

        pthread_t tid;
        if (pthread_create(&tid, NULL, worker, t) == 0) {
            if (count == cap) {
                cap = cap ? cap * 2 : 16;
                pthread_t *new_threads = realloc(threads, cap * sizeof(pthread_t));
                if (!new_threads) {
                    error_log("memory reallocation failed", "threads");
                    worker(t);
                    continue;
                }
                threads = new_threads;
            }
            threads[count++] = tid;
        } else {
            error_log("pthread_create failed", entry->d_name);
            worker(t);
        }
    }

    free(entry);
    closedir(dirp);

    for (size_t i = 0; i < count; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            error_log("pthread_join failed", NULL);
        }
    }
    free(threads);
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <source_dir> <dest_dir>\n", argv[0]);
        return EXIT_FAILURE;
    }

    struct stat st;
    if (stat(argv[1], &st) != 0) {
        fatal_error("source directory stat failed", argv[1]);
    }

    if (!S_ISDIR(st.st_mode)) {
        fatal_error("source is not a directory", argv[1]);
    }

    if (mkdir(argv[2], st.st_mode & 0777) != 0 && errno != EEXIST) {
        error_log("failed to create destination directory", argv[2]);
    }

    traverse_dir(argv[1], argv[2]);
    fprintf(stderr, "Copy completed successfully\n");
    
    return EXIT_SUCCESS;
}
