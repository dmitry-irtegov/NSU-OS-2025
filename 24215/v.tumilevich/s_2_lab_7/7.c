#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>

#define PATH_SIZE 4096
#define COPY_BUF_SIZE 8192

struct thread_args {
    char src_path[PATH_SIZE];
    char dst_path[PATH_SIZE];
};

int open_with_retry(const char *pathname, int flags, mode_t mode) {
    int fd;

    while (1) {
        fd = open(pathname, flags, mode);

        if (fd == -1 && errno == EMFILE) {
            sleep(1);
            continue;
        }

        return fd;
    }
}

DIR *opendir_with_retry(const char *name) {
    DIR *dir;

    while (1) {
        dir = opendir(name);

        if (dir == NULL && errno == EMFILE) {
            sleep(1);
            continue;
        }

        return dir;
    }
}

struct thread_args *make_args(const char *src, const char *dst) {
    struct thread_args *args = malloc(sizeof(struct thread_args));

    if (args == NULL) {
        perror("malloc");
        return NULL;
    }

    snprintf(args->src_path, sizeof(args->src_path), "%s", src);
    snprintf(args->dst_path, sizeof(args->dst_path), "%s", dst);

    return args;
}

int is_thread_object(const char *path) {
    struct stat st;

    if (stat(path, &st) != 0) {
        return 0;
    }

    return S_ISDIR(st.st_mode) || S_ISREG(st.st_mode);
}

size_t count_threads_in_dir(DIR *dir, const char *src_path) {
    struct dirent *entry;
    size_t count = 0;

    rewinddir(dir);
    errno = 0;

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char child_path[PATH_SIZE];

        snprintf(child_path, sizeof(child_path), "%s/%s",
                 src_path, entry->d_name);

        if (is_thread_object(child_path)) {
            count++;
        }
    }

    if (errno != 0) {
        perror("readdir");
    }

    rewinddir(dir);
    return count;
}


void *copy_file_thread(void *arg) {
    struct thread_args *args = (struct thread_args *)arg;

    int fd_in = open_with_retry(args->src_path, O_RDONLY, 0);
    if (fd_in < 0) {
        perror("open source file");
        free(args);
        return NULL;
    }

    struct stat st;
    if (fstat(fd_in, &st) < 0) {
        perror("fstat");
        close(fd_in);
        free(args);
        return NULL;
    }

    int fd_out = open_with_retry(
        args->dst_path,
        O_WRONLY | O_CREAT | O_TRUNC,
        st.st_mode & 0777
    );

    if (fd_out < 0) {
        perror("open destination file");
        close(fd_in);
        free(args);
        return NULL;
    } //fat brother live jornal.ru

    char buf[COPY_BUF_SIZE];
    size_t bytes_read;

    while ((bytes_read = read(fd_in, buf, sizeof(buf))) > 0) {
        size_t total_written = 0;

        while (total_written < bytes_read) {
            size_t bytes_written = write(
                fd_out,
                buf + total_written,
                bytes_read - total_written
            );

            if (bytes_written < 0) {
                perror("write");
                close(fd_in);
                close(fd_out);
                free(args);
                return NULL;
            }

            total_written += bytes_written;
        }
    }

    if (bytes_read < 0) {
        perror("read");
    }

    close(fd_in);
    close(fd_out);
    free(args);

    return NULL;
}

void *copy_dir_thread(void *arg) {
    struct thread_args *args = (struct thread_args *)arg;

    struct stat dir_st;

    if (stat(args->src_path, &dir_st) == 0) {
        if (mkdir(args->dst_path, dir_st.st_mode & 0777) < 0 &&
            errno != EEXIST) {
            perror("mkdir");
            free(args);
            return NULL;
        }
    } else {
        if (mkdir(args->dst_path, 0755) < 0 && errno != EEXIST) {
            perror("mkdir");
            free(args);
            return NULL;
        }
    }

    DIR *dir = opendir_with_retry(args->src_path);
    if (dir == NULL) {
        perror("opendir");
        free(args);
        return NULL;
    }

    size_t max_threads = count_threads_in_dir(dir, args->src_path);

    pthread_t *threads = NULL;

    if (max_threads > 0) {
        threads = malloc(sizeof(pthread_t) * max_threads);

        if (threads == NULL) {
            perror("malloc threads");
            closedir(dir);
            free(args);
            return NULL;
        }
    }

    size_t thread_count = 0;

    errno = 0;

    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char src_child[PATH_SIZE];
        char dst_child[PATH_SIZE];

        snprintf(src_child, sizeof(src_child), "%s/%s",
                 args->src_path, entry->d_name);

        snprintf(dst_child, sizeof(dst_child), "%s/%s",
                 args->dst_path, entry->d_name);

        struct stat entry_st;

        if (stat(src_child, &entry_st) != 0) {
            perror("stat");
            continue;
        }

        if (!S_ISDIR(entry_st.st_mode) && !S_ISREG(entry_st.st_mode)) {
            continue;
        }

        struct thread_args *new_args = make_args(src_child, dst_child);
        if (new_args == NULL) {
            continue;
        }

        pthread_t tid;
        int create_result;

        if (S_ISDIR(entry_st.st_mode)) {
            create_result = pthread_create(&tid, NULL,
                                           copy_dir_thread, new_args);
        } else {
            create_result = pthread_create(&tid, NULL,
                                           copy_file_thread, new_args);
        }

        if (create_result != 0) {
            errno = create_result;
            perror("pthread_create");
            free(new_args);
            continue;
        }

        if (thread_count < max_threads) {
            threads[thread_count++] = tid;
        } else {
            pthread_join(tid, NULL);
        }
    }

    if (errno != 0) {
        perror("readdir");
    }

    closedir(dir);

    for (size_t i = 0; i < thread_count; i++) {
        pthread_join(threads[i], NULL);
    }

    free(threads);
    free(args);

    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr,
                "Использование: %s <исходный_каталог> <целевой_каталог>\n",
                argv[0]);
        return EXIT_FAILURE;
    }

    struct stat src_st;

    if (stat(argv[1], &src_st) != 0) {
        perror("stat source");
        return EXIT_FAILURE;
    }

    if (!S_ISDIR(src_st.st_mode)) {
        fprintf(stderr, "Ошибка: источник должен быть каталогом\n");
        return EXIT_FAILURE;
    }

    struct thread_args *args = make_args(argv[1], argv[2]);
    if (args == NULL) {
        return EXIT_FAILURE;
    }

    pthread_t root_tid;

    int create_result = pthread_create(&root_tid, NULL,
                                       copy_dir_thread, args);

    if (create_result != 0) {
        errno = create_result;
        perror("pthread_create");
        free(args);
        return EXIT_FAILURE;
    }

    pthread_join(root_tid, NULL);

    printf("Копирование завершено.\n");

    return EXIT_SUCCESS;
}


/*
nemohuan@Nautilus:~/Документы/Progs/NSU/OSI/2_semestr/7$ /usr/bin/time -v ./7 src_tree dst_my
end.
        Command being timed: "./7 src_tree dst_my"
        User time (seconds): 0.04
        System time (seconds): 0.40
        Percent of CPU this job got: 64%
        Elapsed (wall clock) time (h:mm:ss or m:ss): 0:00.69    ----
        Average shared text size (kbytes): 0
        Average unshared data size (kbytes): 0
        Average stack size (kbytes): 0
        Average total size (kbytes): 0
        Maximum resident set size (kbytes): 15184
        Average resident set size (kbytes): 0
        Major (requiring I/O) page faults: 0
        Minor (reclaiming a frame) page faults: 3457
        Voluntary context switches: 771
        Involuntary context switches: 1079
        Swaps: 0
        File system inputs: 0
        File system outputs: 418528
        Socket messages sent: 0
        Socket messages received: 0
        Signals delivered: 0
        Page size (bytes): 4096
        Exit status: 0
nemohuan@Nautilus:~/Документы/Progs/NSU/OSI/2_semestr/7$ /usr/bin/time -v cp -R src_tree dst_cp
        Command being timed: "cp -R src_tree dst_cp"
        User time (seconds): 0.00
        System time (seconds): 0.11
        Percent of CPU this job got: 2%
        Elapsed (wall clock) time (h:mm:ss or m:ss): 0:04.38   ----
        Average shared text size (kbytes): 0
        Average unshared data size (kbytes): 0
        Average stack size (kbytes): 0
        Average total size (kbytes): 0
        Maximum resident set size (kbytes): 2772
        Average resident set size (kbytes): 0
        Major (requiring I/O) page faults: 1
        Minor (reclaiming a frame) page faults: 141
        Voluntary context switches: 378
        Involuntary context switches: 110
        Swaps: 0
        File system inputs: 160
        File system outputs: 928
        Socket messages sent: 0
        Socket messages received: 0
        Signals delivered: 0
        Page size (bytes): 4096
        Exit status: 0

*/
