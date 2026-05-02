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
    if (stat(src, &st) != 0) return;

    int src_fd, dst_fd;

    while ((src_fd = open(src, O_RDONLY)) < 0) {
        if (errno == EMFILE) wait_for_fd();
        else if (errno != EINTR) return;
    }

    while ((dst_fd = open(dst, O_WRONLY | O_CREAT | O_TRUNC, st.st_mode & 0777)) < 0) {
        if (errno == EMFILE) wait_for_fd();
        else if (errno != EINTR) { close(src_fd); return; }
    }

    char buf[COPY_BUF_SIZE];
    ssize_t r, w, off;
    
    while ((r = read(src_fd, buf, sizeof(buf))) != 0) {
        if (r < 0) {
            if (errno == EINTR) continue;
            break;
        }
        off = 0;
        while (off < r) {
            w = write(dst_fd, buf + off, r - off);
            if (w < 0) {
                if (errno == EINTR) continue;
                close(src_fd); close(dst_fd); return;
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
        else return;
    }

    long name_max = pathconf(src_dir, _PC_NAME_MAX);
    if (name_max == -1) name_max = 255;
    struct dirent *entry = malloc(sizeof(struct dirent) + name_max + 1);
    if (!entry) { closedir(dirp); return; }

    struct dirent *res;
    pthread_t *threads = NULL;
    size_t count = 0, cap = 0;

    while (1) {
        int rc = readdir_r(dirp, entry, &res);
        if (rc == EMFILE) { wait_for_fd(); continue; }
        if (rc != 0 || !res) break;

        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;

        char *src_path = join_paths(src_dir, entry->d_name);
        char *dst_path = join_paths(dst_dir, entry->d_name);
        struct stat st;

        if (!src_path || !dst_path || stat(src_path, &st) != 0) {
            free(src_path); free(dst_path); continue;
        }

        int is_dir = S_ISDIR(st.st_mode);
        if (is_dir) {
            if (mkdir(dst_path, st.st_mode & 0777) != 0 && errno != EEXIST) {
                free(src_path); free(dst_path); continue;
            }
        } else if (!S_ISREG(st.st_mode)) {
            free(src_path); free(dst_path); continue; 
        }

        struct task *t = malloc(sizeof(*t));
        t->src = src_path; 
        t->dst = dst_path; 
        t->is_dir = is_dir;

        pthread_t tid;
        if (pthread_create(&tid, NULL, worker, t) == 0) {
            if (count == cap) {
                cap = cap ? cap * 2 : 16;
                threads = realloc(threads, cap * sizeof(pthread_t));
            }
            threads[count++] = tid;
        } else {
            worker(t); 
        }
    }

    free(entry);
    closedir(dirp);

    for (size_t i = 0; i < count; i++) pthread_join(threads[i], NULL);
    free(threads);
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <source_dir> <dest_dir>\n", argv[0]);
        return 1;
    }

    struct stat st;
    if (stat(argv[1], &st) != 0 || !S_ISDIR(st.st_mode)) {
        fprintf(stderr, "Source is not a valid directory\n");
        return 1;
    }

    mkdir(argv[2], st.st_mode & 0777);
    traverse_dir(argv[1], argv[2]);
    
    return 0;
}
