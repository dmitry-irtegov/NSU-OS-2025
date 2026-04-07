#include <pthread.h> 
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdlib.h> 
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include "regularhandler.h"
#include "argstruct.h"

int createNewThread(pthread_t **threads, 
                    size_t *threadAmount, 
                    size_t *capacity,
                    void *(*start_routine)(void *),
                    void *arg) {
    size_t size = *(threadAmount); 
    size_t cap = *(capacity);

    if (size + 1 > cap) {
        cap *= 2; 
        pthread_t *newthreads = (pthread_t *) realloc(*(threads), sizeof(pthread_t) * cap); 
        if (newthreads == NULL) {
            perror("cannot realloc the thread array"); 
            return -1; 
        } else {
            *(threads) = newthreads; 
        }
    }

    pthread_t thread; 
    int code = pthread_create(&thread, NULL, start_routine, arg); 
    if (code != 0) {
        char buff[256]; 
        strerror_r(code, buff, sizeof(buff)); 
        fprintf(stderr, "pthread_create: [%s]", buff);
        return -1; 
    }

    (*threads)[size] = thread;
    size++; 
    *(threadAmount) = size; 
    *(capacity) = cap; 

    return 0; 
}

void *copyDirectory(void *arg) {
    if (arg == NULL) return (void *) 0;
    copypaths *cpths = (copypaths *) arg;

    if (cpths->filepath == NULL || cpths->newpath == NULL) {
        free(cpths->filepath);
        free(cpths->newpath);
        free(cpths); 
        return (void *) 0; 
    }

    struct stat statistic;
    if (stat(cpths->filepath, &statistic) != 0) {
        perror("cannot get dir stat");
        free(cpths->filepath);
        free(cpths->newpath);
        free(cpths);
        return (void *) 0;
    }

    if (mkdir(cpths->newpath, statistic.st_mode | S_IRWXU) != 0 && errno != EEXIST) {
        perror("cannot create new directory");
        free(cpths->filepath);
        free(cpths->newpath);
        free(cpths);
        return (void *) 0; 
    }

    DIR *dir = opendir(cpths->filepath);
    while (dir == NULL) {
        if (errno != EMFILE) {
            perror("cannot open directory");
            free(cpths->filepath);
            free(cpths->newpath);
            free(cpths);
            return (void *) 0;
        }
        sleep(1);
        dir = opendir(cpths->filepath);
    }

    struct dirent *entry = (struct dirent *) malloc(sizeof (struct dirent) + pathconf(cpths->filepath, _PC_NAME_MAX) + 1);
    struct dirent *result;

    size_t capacity = 4; 
    pthread_t *threads = (pthread_t *) malloc(sizeof(pthread_t) * capacity);
    size_t threadAmount = 0;

    int readirCode; 

    for (readirCode = readdir_r(dir, entry, &result); 
            readirCode == 0 && result != NULL; 
            readirCode = readdir_r(dir, entry, &result)) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        copypaths *subarg = (copypaths *) malloc (sizeof(copypaths)); 
        if (subarg == NULL) {
            perror("cannot allocate a structure for paths"); 
            continue; 
        }

        subarg->filepath = (char *) malloc(strlen(cpths->filepath) + strlen(entry->d_name) + 2); 
        if (subarg->filepath == NULL) {
            perror("cannot allocate space for filepath"); 
            free(subarg); 
            continue; 
        }

        subarg->newpath = (char *) malloc(strlen(cpths->newpath) + strlen(entry->d_name) + 2); 
        if (subarg->newpath == NULL) {
            perror("cannot allocate space for newpath"); 
            free(subarg); 
            free(subarg->filepath); 
            continue; 
        }

        sprintf(subarg->filepath, "%s/%s", cpths->filepath, entry->d_name); 
        sprintf(subarg->newpath, "%s/%s", cpths->newpath, entry->d_name);

        struct stat itemStat; 
        if (stat(subarg->filepath, &itemStat) != 0) {
            perror("cannot get file stat");
            free(subarg->filepath);
            free(subarg->newpath); 
            free(subarg); 
            continue; 
        }

        if (S_ISREG(itemStat.st_mode)) {
            if (createNewThread(
                &threads, 
                &threadAmount, 
                &capacity, 
                copyRegularFile, 
                (void *) subarg) == -1) {
                free(subarg->filepath);
                free(subarg->newpath); 
                free(subarg);
            }
        } else if (S_ISDIR(itemStat.st_mode)) {
            if (createNewThread(
                &threads, 
                &threadAmount, 
                &capacity, 
                copyDirectory, 
                (void *) subarg) == -1) {
                free(subarg->filepath);
                free(subarg->newpath); 
                free(subarg);
            }
        } else {
            free(subarg->filepath);
            free(subarg->newpath);
            free(subarg); 
        }
    }

    if (readirCode != 0) perror("cannot read directory item"); 

    for (size_t i = 0; i < threadAmount; ++i) {
        int code = pthread_join(threads[i], NULL);
        if (code != 0) {
            char buff[256]; 
            strerror_r(code, buff, sizeof(buff)); 
            fprintf(stderr, "pthread_join: [%s]", buff); 
        }
    }

    free(threads); 
    free(entry);
    closedir(dir);

    free(cpths->filepath);
    free(cpths->newpath); 
    free(cpths); 

    return (void *) 0;
}