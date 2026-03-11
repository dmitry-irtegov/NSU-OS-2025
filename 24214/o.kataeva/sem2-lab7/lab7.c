#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>

#define BUFFER_SIZE 65536
#define handle_error(en, msg) { errno = en; perror(msg); exit(EXIT_FAILURE); }

int active_threads = 0;
pthread_mutex_t thread_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t thread_cond = PTHREAD_COND_INITIALIZER;

typedef struct {
    char src_path[PATH_MAX];
    char dst_path[PATH_MAX];
} thread_args_t;

void thread_started() {
    pthread_mutex_lock(&thread_mutex);
    active_threads++;
    pthread_mutex_unlock(&thread_mutex);
}

void thread_finished() {
    pthread_mutex_lock(&thread_mutex);
    active_threads--;
    if (active_threads == 0) {
        pthread_cond_signal(&thread_cond);
    }
    pthread_mutex_unlock(&thread_mutex);
}

void* copy_file_thread(void *arg) {
    thread_args_t *args = (thread_args_t*)arg;
    char buffer[BUFFER_SIZE];
    struct stat stat_buf;
    int src_fd = -1, dst_fd = -1;

    if (stat(args->src_path, &stat_buf) != 0) {
        perror("stat failed");
        free(args);
        thread_finished();
        return NULL;
    }

    while ((src_fd = open(args->src_path, O_RDONLY)) == -1) {
        if (errno == EMFILE) {
            sleep(1);
        } else {
            perror("open source failed");
            free(args);
            thread_finished();
            return NULL;
        }
    }

    while ((dst_fd = open(args->dst_path, O_WRONLY | O_CREAT | O_TRUNC, stat_buf.st_mode)) == -1) {
        if (errno == EMFILE) {
            sleep(1);
        } else {
            perror("open destination failed");
            close(src_fd);
            free(args);
            thread_finished();
            return NULL;
        }
    }

    ssize_t bytes_read;
    while ((bytes_read = read(src_fd, buffer, sizeof(buffer))) > 0) {
        char *ptr = buffer;
        ssize_t bytes_remaining = bytes_read;
        
        while (bytes_remaining > 0) {
            ssize_t bytes_written = write(dst_fd, ptr, bytes_remaining);
            if (bytes_written <= 0) {
                perror("write failed");
                close(src_fd);
                close(dst_fd);
                free(args);
                thread_finished();
                return NULL;
            }
            ptr += bytes_written;
            bytes_remaining -= bytes_written;
        }
    }

    close(src_fd);
    close(dst_fd);
    free(args);
    thread_finished();
    return NULL;
}

void* process_dir_thread(void *arg);

void create_thread_safe(void* (*start_routine)(void*), thread_args_t *args) {
    pthread_t tid;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED); 

    thread_started();

    while (pthread_create(&tid, &attr, start_routine, args) != 0) {
        if (errno == EAGAIN) {
            sleep(1);
        } else {
            perror("pthread_create failed");
            thread_finished();
            free(args);
            pthread_attr_destroy(&attr);
            return;
        }
    }
    pthread_attr_destroy(&attr);
}

void* process_dir_thread(void *arg) {
    thread_args_t *args = (thread_args_t*)arg;
    DIR *dir = NULL;
    
    if (mkdir(args->dst_path, 0755) != 0 && errno != EEXIST) {
        perror("mkdir failed");
        free(args);
        thread_finished();
        return NULL;
    }

    while ((dir = opendir(args->src_path)) == NULL) {
        if (errno == EMFILE) {
            sleep(1);
        } else {
            perror("opendir failed");
            free(args);
            thread_finished();
            return NULL;
        }
    }

    long name_max = pathconf(args->src_path, _PC_NAME_MAX);
    if (name_max == -1) {
        name_max = 255;
    }
    size_t dirent_size = sizeof(struct dirent) + name_max + 1;
    struct dirent *entry = malloc(dirent_size);
    
    if (!entry) {
        perror("malloc failed");
        closedir(dir);
        free(args);
        thread_finished();
        return NULL;
    }

    struct dirent *result;
    int readdir_res;
    
    while ((readdir_res = readdir_r(dir, entry, &result)) == 0 && result != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        thread_args_t *new_args = malloc(sizeof(thread_args_t));
        if (!new_args) continue;

        if (snprintf(new_args->src_path, PATH_MAX, "%s/%s", args->src_path, entry->d_name) >= PATH_MAX ||
            snprintf(new_args->dst_path, PATH_MAX, "%s/%s", args->dst_path, entry->d_name) >= PATH_MAX) {
            fprintf(stderr, "Path too long\n");
            free(new_args);
            continue;
        }

        struct stat stat_buf;
        if (stat(new_args->src_path, &stat_buf) != 0) {
            free(new_args);
            continue;
        }

        if (S_ISDIR(stat_buf.st_mode)) {
            create_thread_safe(process_dir_thread, new_args);
        } else if (S_ISREG(stat_buf.st_mode)) {
            create_thread_safe(copy_file_thread, new_args);
        } else {
            free(new_args);
        }
    }

    if (readdir_res != 0) {
        perror("readdir_r failed");
    }

    free(entry);
    closedir(dir);
    free(args);
    thread_finished();
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "use: %s source destination\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    struct stat stat_buf;
    if (stat(argv[1], &stat_buf) != 0) {
        handle_error(errno, "stat source failed");
    }

    if (!S_ISDIR(stat_buf.st_mode)) {
        fprintf(stderr, "source is not a directory\n");
        exit(EXIT_FAILURE);
    }

    thread_args_t *root_args = malloc(sizeof(thread_args_t));
    if (realpath(argv[1], root_args->src_path) == NULL) {
        handle_error(errno, "realpath source failed");
    }
    strncpy(root_args->dst_path, argv[2], PATH_MAX);

    create_thread_safe(process_dir_thread, root_args);

    pthread_mutex_lock(&thread_mutex);
    while (active_threads > 0) {
        pthread_cond_wait(&thread_cond, &thread_mutex);
    }
    pthread_mutex_unlock(&thread_mutex);

    printf("Copying completed successfully!\n");

    return 0;
}