#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>

#define BUFFER_SIZE 8192

pthread_mutex_t fd_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t fd_cond = PTHREAD_COND_INITIALIZER;

typedef struct {
    char *src_path;
    char *dst_path;
} file_task_t;

typedef struct {
    char *src_dir;
    char *dst_dir;
} dir_task_t;

void* dir_worker(void *arg);

void* file_worker(void *arg) {
    file_task_t *task = (file_task_t*)arg;
    int src_fd = -1;
    int dst_fd = -1;
    struct stat st;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read, bytes_written;
    char *ptr;
    int open_success = 0;

    pthread_mutex_lock(&fd_mutex);
    
    while (!open_success) {
        src_fd = open(task->src_path, O_RDONLY, 0);
        if (src_fd < 0) {
            if (errno == EMFILE || errno == ENFILE) {
                pthread_cond_wait(&fd_cond, &fd_mutex);
                continue;
            }
            pthread_mutex_unlock(&fd_mutex);
            fprintf(stderr, "Error opening source file: %s\n", strerror(errno));
            free(task->src_path);
            free(task->dst_path);
            free(task);
            return NULL;
        }

        if (fstat(src_fd, &st) < 0) {
            close(src_fd);
            pthread_cond_signal(&fd_cond);
            pthread_mutex_unlock(&fd_mutex);
            fprintf(stderr, "Error fstat source file: %s\n", strerror(errno));
            free(task->src_path);
            free(task->dst_path);
            free(task);
            return NULL;
        }

        dst_fd = open(task->dst_path, O_WRONLY | O_CREAT | O_TRUNC, st.st_mode);
        if (dst_fd < 0) {
            if (errno == EMFILE || errno == ENFILE) {
                close(src_fd);
                pthread_cond_signal(&fd_cond);
                pthread_cond_wait(&fd_cond, &fd_mutex);
                continue;
            }
            close(src_fd);
            int err = pthread_cond_signal(&fd_cond);
            if (err != 0) {
                fprintf(stderr, "pthread_cond_signal failed: %s\n", strerror(err));
            } 
            pthread_mutex_unlock(&fd_mutex);
            fprintf(stderr, "Error creating destination file: %s\n", strerror(errno));
            free(task->src_path);
            free(task->dst_path);
            free(task);
            return NULL;
        }

        open_success = 1;
    }
    
    pthread_mutex_unlock(&fd_mutex);

    while ((bytes_read = read(src_fd, buffer, BUFFER_SIZE)) > 0) {
        ptr = buffer;
        while (bytes_read > 0) {
            bytes_written = write(dst_fd, ptr, bytes_read);
            if (bytes_written < 0) {
                fprintf(stderr, "Write error: %s\n", strerror(errno));
                break;
            }
            bytes_read -= bytes_written;
            ptr += bytes_written;
        }
    }

    if (bytes_read < 0) {
        fprintf(stderr, "Read error: %s\n", strerror(errno));
    }

    close(src_fd);
    close(dst_fd);
    pthread_cond_signal(&fd_cond);
    pthread_cond_signal(&fd_cond);

    free(task->src_path);
    free(task->dst_path);
    free(task);
    return NULL;
}

void* dir_worker(void *arg) {
    dir_task_t *task = (dir_task_t*)arg;
    DIR *dir = NULL;

    pthread_mutex_lock(&fd_mutex);
    
    while ((dir = opendir(task->src_dir)) == NULL) {
        if (errno == EMFILE || errno == ENFILE) {
            pthread_cond_wait(&fd_cond, &fd_mutex);
            continue;
        }
        break;
    }
    
    if (dir == NULL) {
        pthread_mutex_unlock(&fd_mutex);
        fprintf(stderr, "Error opendir: %s\n", strerror(errno));
        free(task->src_dir);
        free(task->dst_dir);
        free(task);
        return NULL;
    }
    
    pthread_mutex_unlock(&fd_mutex);

    long name_max = pathconf(task->src_dir, _PC_NAME_MAX);
    if (name_max < 0) {
        name_max = 255; 
    }
    size_t entry_size = sizeof(struct dirent) + name_max + 1;
    struct dirent *entry_buf = malloc(entry_size);
    
    if (!entry_buf) {
        fprintf(stderr, "Malloc entry_buf failed: %s\n", strerror(errno));
        closedir(dir);
        pthread_cond_signal(&fd_cond);
        free(task->src_dir);
        free(task->dst_dir);
        free(task);
        return NULL;
    }

    pthread_t child_threads[512];
    int thread_count = 0;
    struct dirent *result;

    while (readdir_r(dir, entry_buf, &result) == 0 && result != NULL) {
        if (strcmp(result->d_name, ".") == 0 || strcmp(result->d_name, "..") == 0) {
            continue;
        }

        if (thread_count >= 512) {
            fprintf(stderr, "Too many files in directory %s, limit is 512\n", task->src_dir);
            break;
        }

        char *sub_src = malloc(strlen(task->src_dir) + strlen(result->d_name) + 2);
        char *sub_dst = malloc(strlen(task->dst_dir) + strlen(result->d_name) + 2);
        if (!sub_src || !sub_dst) {
            fprintf(stderr, "Malloc sub-paths failed: %s\n", strerror(errno));
            free(sub_src); 
            free(sub_dst);
            continue;
        }
        
        if (!sub_src || !sub_dst) {
            fprintf(stderr, "Malloc failed for sub-paths: %s\n", strerror(errno));
            free(sub_src);
            free(sub_dst);
            continue;
        }

        sprintf(sub_src, "%s/%s", task->src_dir, result->d_name);
        sprintf(sub_dst, "%s/%s", task->dst_dir, result->d_name);

        struct stat st;
        if (stat(sub_src, &st) < 0) {
            fprintf(stderr, "Stat error (could be a broken symlink, ignoring): %s\n", strerror(errno));
            free(sub_src); 
            free(sub_dst);
            continue;
        }

        if (S_ISREG(st.st_mode)) {
            file_task_t *f_task = malloc(sizeof(file_task_t));
            if (!f_task) {
                fprintf(stderr, "Malloc f_task failed: %s\n", strerror(errno));
                free(sub_src); 
                free(sub_dst);
                continue;
            }
            f_task->src_path = sub_src;
            f_task->dst_path = sub_dst;

            int err = pthread_create(&child_threads[thread_count], NULL, file_worker, f_task); 
            if (err == 0) {
                thread_count++;
            } else {
                fprintf(stderr, "Failed to create file thread: %s\n", strerror(err));
                free(sub_src); 
                free(sub_dst); 
                free(f_task);
            }

        } else if (S_ISDIR(st.st_mode)) {
            if (mkdir(sub_dst, st.st_mode) < 0 && errno != EEXIST) {
                fprintf(stderr, "Mkdir error: %s\n", strerror(errno));
                free(sub_src); 
                free(sub_dst);
                continue;
            }

            dir_task_t *d_task = malloc(sizeof(dir_task_t));
            if (!d_task) {
                fprintf(stderr, "Malloc d_task failed: %s\n", strerror(errno));
                free(sub_src); free(sub_dst);
                continue;
            }
            d_task->src_dir = sub_src;
            d_task->dst_dir = sub_dst;

            int err = pthread_create(&child_threads[thread_count], NULL, dir_worker, d_task); 
            if (err == 0) {
                thread_count++;
            } else {
                fprintf(stderr, "Failed to create dir thread: %s\n", strerror(err));
                free(sub_src); 
                free(sub_dst);
                free(d_task);
            }
        } else {
            free(sub_src);
            free(sub_dst);
        }
    }

    for (int i = 0; i < thread_count; i++) {
        int err = pthread_join(child_threads[i], NULL);
        if (err != 0) {
            fprintf(stderr, "Failed joining thread %d: %s\n", i, strerror(err));
        }
    }

    free(entry_buf);
    closedir(dir);
    
    int err = pthread_cond_signal(&fd_cond);
    if (err != 0) {
        fprintf(stderr, "pthread_cond_signal failed: %s\n", strerror(err));
    }

    free(task->src_dir);
    free(task->dst_dir);
    free(task);
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <source_dir> <destination_dir>\n", argv[0]);
        exit(1);
    }

    struct stat st;
    if (stat(argv[1], &st) < 0 || !S_ISDIR(st.st_mode)) {
        fprintf(stderr, "Error: Source path is not a directory or doesn't exist.\n");
        exit(1);
    }

    if (mkdir(argv[2], st.st_mode) < 0 && errno != EEXIST) {
        fprintf(stderr, "Error creating root destination directory: %s\n", strerror(errno));
        exit(1);
    }

    dir_task_t *root_task = malloc(sizeof(dir_task_t));
    if (!root_task) {
        fprintf(stderr, "Error allocating root task: %s\n", strerror(errno));
        exit(1);
    }

    root_task->src_dir = strdup(argv[1]);
    root_task->dst_dir = strdup(argv[2]);
    
    if (!root_task->src_dir || !root_task->dst_dir) {
        fprintf(stderr, "Error duplicating path strings: %s\n", strerror(errno));
        free(root_task->src_dir);
        free(root_task->dst_dir);
        free(root_task);
        exit(1);
    }

    pthread_t main_dir_thread;
    int err = pthread_create(&main_dir_thread, NULL, dir_worker, root_task);
    if (err != 0) {
        fprintf(stderr, "Error creating initial thread: %s\n", strerror(err));
        free(root_task->src_dir);
        free(root_task->dst_dir);
        free(root_task);
        exit(1);
    }

    err = pthread_join(main_dir_thread, NULL);
    if (err != 0) {
        fprintf(stderr, "pthread_join error: %s\n", strerror(err));
        free(root_task->src_dir);
        free(root_task->dst_dir);
        free(root_task);
        exit(1);
    }

    printf("Copying process completed successfully.\n");
    return 0;
}
