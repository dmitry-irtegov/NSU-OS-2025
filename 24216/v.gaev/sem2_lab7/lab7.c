#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>

#define COPY_BUFFER_SIZE 8192

typedef struct {
    char *src_path;
    char *dst_path;
} thread_args_t;

typedef struct thread_node {
    pthread_t tid;
    struct thread_node *next;
} thread_node_t;

int safe_open(const char *pathname, int flags, mode_t mode) {
    int fd;
    while (1) {
        fd = open(pathname, flags, mode);
        if (fd >= 0) {
            return fd;
        }
        if (errno == EMFILE || errno == ENFILE) {
            sleep(1);
        } else {
            return -1;
        }
    }
}

DIR* safe_opendir(const char *name) {
    DIR *dir;
    while (1) {
        dir = opendir(name);
        if (dir != NULL) {
            return dir;
        }
        if (errno == EMFILE || errno == ENFILE) {
            sleep(1);
        } else {
            return NULL;
        }
    }
}

void* copy_file_thread(void *arg) {
    thread_args_t *args = (thread_args_t *)arg;
    int fd_in = -1, fd_out = -1;
    ssize_t bytes_read, bytes_written;
    char buffer[COPY_BUFFER_SIZE];
    struct stat st;

    if (lstat(args->src_path, &st) == -1) {
        fprintf(stderr, "Ошибка stat для файла %s: %s\n", args->src_path, strerror(errno));
    } else {
        fd_in = safe_open(args->src_path, O_RDONLY, 0);
        if (fd_in < 0) {
            fprintf(stderr, "Ошибка открытия исходного файла %s: %s\n", args->src_path, strerror(errno));
        } else {
            fd_out = safe_open(args->dst_path, O_WRONLY | O_CREAT | O_TRUNC, st.st_mode & 0777);
            if (fd_out < 0) {
                fprintf(stderr, "Ошибка создания целевого файла %s: %s\n", args->dst_path, strerror(errno));
            } else {
                int write_error = 0;
                while ((bytes_read = read(fd_in, buffer, sizeof(buffer))) > 0) {
                    ssize_t total_written = 0;
                    while (total_written < bytes_read) {
                        bytes_written = write(fd_out, buffer + total_written, bytes_read - total_written);
                        if (bytes_written < 0) {
                            if (errno == EINTR) continue;
                            fprintf(stderr, "Ошибка записи в файл %s: %s\n", args->dst_path, strerror(errno));
                            write_error = 1;
                            break;
                        }
                        total_written += bytes_written;
                    }
                    if (write_error) {
                        break;
                    }
                }

                if (bytes_read < 0) {
                    fprintf(stderr, "Ошибка чтения из файла %s: %s\n", args->src_path, strerror(errno));
                }
            }
        }
    }

    if (fd_in >= 0) close(fd_in);
    if (fd_out >= 0) close(fd_out);
    free(args->src_path);
    free(args->dst_path);
    free(args);
    return NULL;
}

void* copy_dir_thread(void *arg) {
    thread_args_t *args = (thread_args_t *)arg;
    DIR *dir = NULL;
    struct dirent *entry = NULL;
    struct dirent *result = NULL;
    long name_max;
    size_t dirent_size;
    struct stat st;
    thread_node_t *threads = NULL;

    if (lstat(args->src_path, &st) == -1) {
        fprintf(stderr, "Ошибка stat для каталога %s: %s\n", args->src_path, strerror(errno));
    } else if (mkdir(args->dst_path, st.st_mode & 0777) == -1 && errno != EEXIST) {
        fprintf(stderr, "Ошибка создания каталога %s: %s\n", args->dst_path, strerror(errno));
    } else {
        dir = safe_opendir(args->src_path);
        if (dir == NULL) {
            fprintf(stderr, "Ошибка открытия каталога %s: %s\n", args->src_path, strerror(errno));
        } else {
            name_max = pathconf(args->src_path, _PC_NAME_MAX);
            if (name_max == -1) {
                name_max = 255;
            }
            dirent_size = sizeof(struct dirent) + name_max + 1;
            entry = (struct dirent *)malloc(dirent_size);
            
            if (!entry) {
                fprintf(stderr, "Ошибка выделения памяти для dirent\n");
            } else {
                while (readdir_r(dir, entry, &result) == 0 && result != NULL) {
                    if (strcmp(result->d_name, ".") == 0 || strcmp(result->d_name, "..") == 0) {
                        continue;
                    }

                    size_t src_len = strlen(args->src_path) + strlen(result->d_name) + 2;
                    size_t dst_len = strlen(args->dst_path) + strlen(result->d_name) + 2;
                    
                    char *new_src = malloc(src_len);
                    char *new_dst = malloc(dst_len);
                    
                    snprintf(new_src, src_len, "%s/%s", args->src_path, result->d_name);
                    snprintf(new_dst, dst_len, "%s/%s", args->dst_path, result->d_name);

                    struct stat child_st;
                    if (lstat(new_src, &child_st) == -1) {
                        fprintf(stderr, "Ошибка lstat %s: %s\n", new_src, strerror(errno));
                        free(new_src);
                        free(new_dst);
                        continue;
                    }

                    thread_args_t *child_args = malloc(sizeof(thread_args_t));
                    child_args->src_path = new_src;
                    child_args->dst_path = new_dst;

                    pthread_t tid;
                    int create_err = 0;

                    if (S_ISLNK(child_st.st_mode)) {
                        free(new_src);
                        free(new_dst);
                        free(child_args);
                        continue;
                    } else if (S_ISDIR(child_st.st_mode)) {
                        create_err = pthread_create(&tid, NULL, copy_dir_thread, child_args);
                    } else if (S_ISREG(child_st.st_mode)) {
                        create_err = pthread_create(&tid, NULL, copy_file_thread, child_args);
                    } else {
                        free(new_src);
                        free(new_dst);
                        free(child_args);
                        continue;
                    }

                    if (create_err != 0) {
                        fprintf(stderr, "Ошибка создания нити для %s: %s\n", new_src, strerror(create_err));
                        free(new_src);
                        free(new_dst);
                        free(child_args);
                    } else {
                        thread_node_t *node = malloc(sizeof(thread_node_t));
                        node->tid = tid;
                        node->next = threads;
                        threads = node;
                    }
                }
            }
        }
    }

    while (threads != NULL) {
        pthread_join(threads->tid, NULL);
        thread_node_t *tmp = threads;
        threads = threads->next;
        free(tmp);
    }

    if (dir) closedir(dir);
    if (entry) free(entry);
    free(args->src_path);
    free(args->dst_path);
    free(args);
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Использование: %s <исходный_каталог> <целевой_каталог>\n", argv[0]);
        return 1;
    }

    thread_args_t *args = malloc(sizeof(thread_args_t));
    args->src_path = strdup(argv[1]);
    args->dst_path = strdup(argv[2]);

    pthread_t tid;
    if (pthread_create(&tid, NULL, copy_dir_thread, args) != 0) {
        fprintf(stderr, "Ошибка создания корневой нити\n");
        return 1;
    }

    pthread_join(tid, NULL);
    return 0;
}
