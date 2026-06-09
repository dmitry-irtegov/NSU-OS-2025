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

#ifndef NAME_MAX
#define NAME_MAX 255
#endif

#define COPY_BUFFER_SIZE 8192
#define EMFILE_SLEEP_SECONDS 1

typedef struct {
    char* src_path;
    char* dst_path;
} CopyTask;

typedef struct {
    pthread_t* items;
    size_t count;
    size_t capacity;
} ThreadList;

void free_task(CopyTask* task) {
    if (task == NULL) return;

    free(task->src_path);
    free(task->dst_path);
    free(task);
}

char* duplicate_string(const char* str) {
    size_t len = strlen(str) + 1;
    char* result = (char*)malloc(len);
    if (result == NULL) return NULL;

    memcpy(result, str, len);
    return result;
}

char* join_path(const char* dir, const char* name) {
    size_t dir_len = strlen(dir);
    size_t name_len = strlen(name);
    int needs_slash = dir_len > 0 && dir[dir_len - 1] != '/';
    char* result = (char*)malloc(dir_len + needs_slash + name_len + 1);

    if (result == NULL) return NULL;

    memcpy(result, dir, dir_len);
    if (needs_slash) result[dir_len] = '/';
    memcpy(result + dir_len + needs_slash, name, name_len + 1);

    return result;
}

CopyTask* create_task(const char* src_path, const char* dst_path) {
    CopyTask* task = (CopyTask*)malloc(sizeof(CopyTask));

    if (task == NULL) return NULL;

    task->src_path = duplicate_string(src_path);
    task->dst_path = duplicate_string(dst_path);
    if (task->src_path == NULL || task->dst_path == NULL) {
        free_task(task);
        return NULL;
    }

    return task;
}

int add_thread(ThreadList* list, pthread_t thread) {
    if (list->count == list->capacity) {
        size_t new_capacity = list->capacity == 0 ? 8 : list->capacity * 2;
        pthread_t* new_items = (pthread_t*)realloc(list->items, new_capacity * sizeof(pthread_t));
        if (new_items == NULL) return -1;

        list->items = new_items;
        list->capacity = new_capacity;
    }

    list->items[list->count] = thread;
    list->count++;
    return 0;
}

DIR* open_directory_with_retry(const char* path) {
    for (;;) {
        DIR* dir = opendir(path);
        if (dir != NULL || errno != EMFILE) return dir;
        sleep(EMFILE_SLEEP_SECONDS);
    }
}

int open_file_with_retry(const char* path, int flags, mode_t mode) {
    for (;;) {
        int fd = open(path, flags, mode);
        if (fd != -1) return fd;
        if (errno == EINTR) continue;
        if (errno != EMFILE) return fd;
        sleep(EMFILE_SLEEP_SECONDS);
    }
}

int write_all(int fd, const char* buffer, ssize_t size) {
    ssize_t total_written = 0;

    while (total_written < size) {
        ssize_t written = write(fd, buffer + total_written, size - total_written);
        if (written == -1) {
            if (errno == EINTR) continue;
            return -1;
        }
        total_written += written;
    }

    return 0;
}

void* copy_file(void* arg) {
    CopyTask* task = (CopyTask*)arg;
    char buffer[COPY_BUFFER_SIZE];
    struct stat src_stat = {0};

    if (lstat(task->src_path, &src_stat) == -1) {
        fprintf(stderr, "lstat %s: %s\n", task->src_path, strerror(errno));
        free_task(task);
        return NULL;
    }

    int src_fd = open_file_with_retry(task->src_path, O_RDONLY, 0);
    if (src_fd == -1) {
        fprintf(stderr, "open %s: %s\n", task->src_path, strerror(errno));
        free_task(task);
        return NULL;
    }

    int dst_fd = open_file_with_retry(task->dst_path, O_WRONLY | O_CREAT | O_TRUNC, src_stat.st_mode & 0777);
    if (dst_fd == -1) {
        fprintf(stderr, "open %s: %s\n", task->dst_path, strerror(errno));
        close(src_fd);
        free_task(task);
        return NULL;
    }

    ssize_t bytes_read = 0;
    for (;;) {
        bytes_read = read(src_fd, buffer, sizeof(buffer));
        if (bytes_read == 0) {
            break;
        }
        if (bytes_read == -1) {
            if (errno == EINTR) continue;
            fprintf(stderr, "read %s: %s\n", task->src_path, strerror(errno));
            break;
        }
        if (write_all(dst_fd, buffer, bytes_read) == -1) {
            fprintf(stderr, "write %s: %s\n", task->dst_path, strerror(errno));
            break;
        }
    }

    close(src_fd);
    close(dst_fd);
    free_task(task);
    return NULL;
}

int create_directory_if_needed(const char* path, mode_t mode) {
    if (mkdir(path, mode) == 0) {
        return 0;
    }
    if (errno == EEXIST) {
        struct stat path_stat = {0};
        if (lstat(path, &path_stat) == 0 && S_ISDIR(path_stat.st_mode)) {
            return 0;
        }
    }

    return -1;
}

int start_thread(ThreadList* threads, void* (*routine)(void*), CopyTask* task) {
    pthread_t thread;
    int status = pthread_create(&thread, NULL, routine, task);

    if (status != 0) {
        fprintf(stderr, "pthread_create: %s\n", strerror(status));
        return -1;
    }

    if (add_thread(threads, thread) == -1) {
        fprintf(stderr, "malloc: failed to save thread id\n");
        pthread_join(thread, NULL);
        return 0;
    }

    return 0;
}

void join_created_threads(ThreadList* threads) {
    for (size_t i = 0; i < threads->count; i++) {
        int status = pthread_join(threads->items[i], NULL);
        if (status != 0) {
            fprintf(stderr, "pthread_join: %s\n", strerror(status));
        }
    }

    free(threads->items);
}

int read_next_entry(DIR* dir, struct dirent* entry, struct dirent** result) {
    for (;;) {
        int status = readdir_r(dir, entry, result);
        if (status != EMFILE) return status;
        sleep(EMFILE_SLEEP_SECONDS);
    }
}

void* copy_directory(void* arg) {
    CopyTask* task = (CopyTask*)arg;
    ThreadList threads = {NULL, 0, 0};
    DIR* dir = open_directory_with_retry(task->src_path);
    if (dir == NULL) {
        fprintf(stderr, "opendir %s: %s\n", task->src_path, strerror(errno));
        free_task(task);
        return NULL;
    }

    long name_max = pathconf(task->src_path, _PC_NAME_MAX);
    if (name_max == -1) {
        name_max = NAME_MAX;
    }

    size_t entry_size = sizeof(struct dirent) + (size_t)name_max + 1;
    struct dirent* entry = (struct dirent*)malloc(entry_size);
    if (entry == NULL) {
        fprintf(stderr, "malloc: failed to allocate dirent buffer\n");
        closedir(dir);
        free_task(task);
        return NULL;
    }

    for (;;) {
        struct dirent* result = NULL;
        int status = read_next_entry(dir, entry, &result);
        if (status != 0) {
            fprintf(stderr, "readdir_r %s: %s\n", task->src_path, strerror(status));
            break;
        }
        if (result == NULL) break;

        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;

        char* src_child = join_path(task->src_path, entry->d_name);
        char* dst_child = join_path(task->dst_path, entry->d_name);
        struct stat file_stat = {0};
        void* (*routine)(void*) = NULL;

        if (src_child == NULL || dst_child == NULL) {
            fprintf(stderr, "malloc: failed to build path\n");
            free(src_child);
            free(dst_child);
            continue;
        }

        if (lstat(src_child, &file_stat) == -1) {
            fprintf(stderr, "lstat %s: %s\n", src_child, strerror(errno));
            free(src_child);
            free(dst_child);
            continue;
        }

        if (S_ISDIR(file_stat.st_mode)) {
            if (create_directory_if_needed(dst_child, file_stat.st_mode & 0777) == -1) {
                fprintf(stderr, "mkdir %s: %s\n", dst_child, strerror(errno));
                free(src_child);
                free(dst_child);
                continue;
            }
            routine = copy_directory;
        } else if (S_ISREG(file_stat.st_mode)) {
            routine = copy_file;
        } else {
            free(src_child);
            free(dst_child);
            continue;
        }

        CopyTask* child_task = create_task(src_child, dst_child);
        if (child_task == NULL) {
            fprintf(stderr, "malloc: failed to create task\n");
            free(src_child);
            free(dst_child);
            continue;
        }
        if (start_thread(&threads, routine, child_task) == -1) {
            free_task(child_task);
        }

        free(src_child);
        free(dst_child);
    }

    free(entry);
    closedir(dir);
    join_created_threads(&threads);
    free_task(task);
    return NULL;
}

int main(int argc, char** argv) {
    struct stat src_stat = {0};

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <source_dir> <target_dir>\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (lstat(argv[1], &src_stat) == -1) {
        fprintf(stderr, "lstat %s: %s\n", argv[1], strerror(errno));
        return EXIT_FAILURE;
    }
    if (!S_ISDIR(src_stat.st_mode)) {
        fprintf(stderr, "%s is not a directory\n", argv[1]);
        return EXIT_FAILURE;
    }

    if (create_directory_if_needed(argv[2], src_stat.st_mode & 0777) == -1) {
        fprintf(stderr, "mkdir %s: %s\n", argv[2], strerror(errno));
        return EXIT_FAILURE;
    }

    CopyTask* root_task = create_task(argv[1], argv[2]);
    if (root_task == NULL) {
        fprintf(stderr, "malloc: failed to create root task\n");
        return EXIT_FAILURE;
    }

    copy_directory(root_task);
    return EXIT_SUCCESS;
}
