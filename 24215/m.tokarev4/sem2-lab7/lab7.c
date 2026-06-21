#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>

typedef struct Thread{
    pthread_t thread;
    struct Thread* nextthread;
}Thread;

typedef struct {
    char* source;
    char* dest;
}Infodirs;

char* createpath(const char* dir, const char* name) {
    size_t len = strlen(dir) + strlen(name) + 2;
    char* path = (char*)malloc(len);
    snprintf(path, len, "%s/%s", dir, name);
    return path;
}

void* copyfile(void* args) {
    Infodirs* info = (Infodirs*)args;
    int fd_source, fd_dest;

    while ((fd_source = open(info->source, O_RDONLY)) == -1) {
        if (errno == EMFILE || errno == ENFILE) {
            usleep(10000);
        }
        else {
            perror("Fail to open source file");
            free(info->dest);
            free(info->source);
            free(info);
            return NULL;
        }
    }
    while ((fd_dest = open(info->dest, O_CREAT | O_WRONLY | O_TRUNC, 0666)) == -1) {
        if (errno == EMFILE || errno == ENFILE) {
            usleep(10000);
        }
        else {
            perror("Fail to open dest file");
            close(fd_source);
            free(info->dest);
            free(info->source);
            free(info);
            return NULL;
        }
    }

    char buf[8092];
    ssize_t readen, writen;
    while ((readen = read(fd_source, buf, 8092)) > 0) {
        ssize_t writen_now = 0;
        while (writen_now < readen) {
            writen = write(fd_dest, buf + writen_now, readen - writen_now);
            if (writen < 0) {
                perror("Fail to write");
            }
            writen_now += writen;
        }
    }
    close(fd_source);
    close(fd_dest);

    free(info->dest);
    free(info->source);
    free(info);

    return NULL;
}


void* copydir(void* args) {
    Infodirs* info = (Infodirs*)args;
    DIR* dir;
    Thread* headthread = NULL;

    mkdir(info->dest, 0777);
    
    while ((dir = opendir(info->source)) == NULL) {
        if (errno == EMFILE || errno == ENFILE) {
            usleep(10000);
        }
        else {
            perror("Fail to open dir");
            free(info->dest);
            free(info->source);
            free(info);
            return NULL;
        }
    }
    
    long len = pathconf(info->source, _PC_NAME_MAX);
    if (len == -1) {
        len = 255;
    }

    size_t size = sizeof(struct dirent) + len + 1;
    struct dirent* buf = (struct dirent*)malloc(size);
    struct dirent* res;

    while (readdir_r(dir, buf, &res) == 0 && res != NULL) {
        if (strcmp(res->d_name, ".") == 0 || strcmp(res->d_name, "..") == 0) {
            continue;
        }
        char* source = createpath(info->source, res->d_name);
        char* dest = createpath(info->dest, res->d_name);

        struct stat st;
        if (stat(source, &st) == 0) {
            Infodirs* info = (Infodirs*)malloc(sizeof(Infodirs));
            info->source = source;
            info->dest = dest;

            pthread_t newthread;
            int marker = -1;
            
            if (S_ISDIR(st.st_mode)) {
                marker = pthread_create(&newthread, NULL, copydir, info);
            }
            else if (S_ISREG(st.st_mode)) {
                marker = pthread_create(&newthread, NULL, copyfile, info);
            }
            else {
                free(source);
                free(dest);
                free(info);
            }

            if (marker == 0) {
                Thread* node = (Thread*)malloc(sizeof(Thread));
                node->thread = newthread;
                node->nextthread = headthread;
                headthread = node;
            }
        }
        else {
            free(source);
            free(dest);
        }
    }
    
    free(buf);
    closedir(dir);

    Thread* current = headthread;
    while (current != NULL) {
        pthread_join(current->thread, NULL);
        Thread* tmp = current;
        current = current->nextthread;
        free(tmp);
    }

    free(info->dest);
    free(info->source);
    free(info);

    return NULL;
}

int main(int argn, char *argv[]) {
    if (argn != 3) {
        printf("Need to wrtite \"source_dir\" \"dest_dir\"\n");
        return EXIT_FAILURE;
    }

    struct stat st;

    if (stat(argv[1], &st) != 0 || !S_ISDIR(st.st_mode)) {
        printf("source_dir must be\n");
        return EXIT_FAILURE;
    }
    pthread_t main;
    Infodirs* info = (Infodirs*)malloc(sizeof(Infodirs));
    info->source = strdup(argv[1]);
    info->dest = strdup(argv[2]);
    if (pthread_create(&main, NULL, copydir, info)) {
        perror("Error creating main thread\n");
        free(info->source);
        free(info->dest);
        free(info);
        return EXIT_FAILURE;
    }
    pthread_join(main, NULL);
    printf("Copying completed successfully\n");
    
    return 0;
}

