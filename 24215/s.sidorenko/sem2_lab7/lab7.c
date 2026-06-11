#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <limits.h>

#define BUF_SIZE 8192

typedef struct {
    char src[PATH_MAX];
    char dst[PATH_MAX];
} Task;

void* copy_tree(void* arg) {
    Task* task = (Task*)arg;

    DIR* dir;

    while ((dir = opendir(task->src)) == NULL) {
        if (errno == EMFILE) {
            sleep(3);
            continue;
        }
        perror("opendir");
        free(task);
        return NULL;
    }

    long name_max = pathconf(task->src, _PC_NAME_MAX);
    if (name_max == -1) {
        name_max = 255;
    }

    size_t buf_size = sizeof(struct dirent) + name_max + 1;
    struct dirent* entry = malloc(buf_size);
    struct dirent* result;

    if (!entry) {
        perror("malloc");
        closedir(dir);
        free(task);
        return NULL;
    }

    pthread_t threads[1024];
    int thread_count = 0;

    while (1) {
        int ret = readdir_r(dir, entry, &result);

        if (ret == EMFILE) {
            sleep(3);
            continue;
        }

        if (ret != 0) {
            errno = ret;
            perror("readdir_r");
            break;
        }

        if (result == NULL) {
            break;
        }

        if (strcmp(result->d_name, ".") == 0 || strcmp(result->d_name, "..") == 0) {
            continue;
        }

        char src_path[PATH_MAX];
        char dst_path[PATH_MAX];

        snprintf(src_path, PATH_MAX, "%s/%s", task->src, result->d_name);
        snprintf(dst_path, PATH_MAX, "%s/%s", task->dst, result->d_name);

        struct stat st;

        if (stat(src_path, &st) == -1) {
            perror("stat");
            continue;
        }

        Task* new_task = malloc(sizeof(Task));
        if (!new_task) {
            perror("malloc");
            continue;
        }

        strcpy(new_task->src, src_path);
        strcpy(new_task->dst, dst_path);

        if (S_ISDIR(st.st_mode)) {

            if (mkdir(dst_path, st.st_mode & 0777) == -1 && errno != EEXIST) {
                perror("mkdir");
                free(new_task);
                continue;
            }

            if (pthread_create(&threads[thread_count], NULL, copy_tree, new_task) != 0) {
                perror("pthread_create");
                free(new_task);
                continue;
            }

            thread_count++;

        } else if (S_ISREG(st.st_mode)) {

            if (pthread_create(&threads[thread_count], NULL, copy_file, new_task) != 0) {
                perror("pthread_create");
                free(new_task);
                continue;
            }

            thread_count++;

        } else {
            free(new_task);
        }
    }

    for (int i = 0; i < thread_count; i++) {
        pthread_join(threads[i], NULL);
    }

    free(entry);
    closedir(dir);
    free(task);

    return NULL;
}


void* copy_file(void* arg) {
    Task* task = (Task*)arg;

    int in_fd;

    while ((in_fd = open(task->src, O_RDONLY)) == -1) {
        if (errno == EMFILE) {
            sleep(3);
            continue;
        }

        perror("open source");
        free(task);
        return NULL;
    }

    struct stat st;

    if (stat(task->src, &st) == -1) {
        perror("stat");
        close(in_fd);
        free(task);
        return NULL;
    }

    int out_fd;

    while ((out_fd = open(task->dst, O_WRONLY | O_CREAT | O_TRUNC, st.st_mode & 0777)) == -1) {

        if (errno == EMFILE) {
            sleep(3);
            continue;
        }

        perror("open destination");
        close(in_fd);
        free(task);
        return NULL;
    }

    char buffer[BUF_SIZE];
    ssize_t bytes_read;

    while ((bytes_read = read(in_fd, buffer, BUF_SIZE)) > 0) {
        ssize_t written = 0;

        while (written < bytes_read) {
            ssize_t res = write(out_fd, buffer + written, bytes_read - written);

            if (res == -1) {
                perror("write");

                close(in_fd);
                close(out_fd);

                free(task);
                return NULL;
            }

            written += res;
        }
    }

    if (bytes_read == -1) {
        perror("read");
    }

    close(in_fd);
    close(out_fd);

    free(task);

    return NULL;
}


int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <source_dir> <destination_dir>\n", argv[0]);
        return 1;
    }

    struct stat st;

    if (stat(argv[1], &st) == -1) {
        perror("stat source");
        return 1;
    }

    if (!S_ISDIR(st.st_mode)) {
        fprintf(stderr, "Source is not a directory\n");
        return 1;
    }

    if (mkdir(argv[2], st.st_mode & 0777) == -1 && errno != EEXIST) {
        perror("destination");
        return 1;
    }

    Task* root_task = malloc(sizeof(Task));
    if (!root_task) {
        perror("malloc");
        return 1;
    }

    strcpy(root_task->src, argv[1]);
    strcpy(root_task->dst, argv[2]);

    pthread_t root_thread;

    if (pthread_create(&root_thread, NULL, copy_tree, root_task) != 0) {
        perror("pthread_create");
        free(root_task);
        return 1;
    }

    pthread_join(root_thread, NULL);

    return 0;
}