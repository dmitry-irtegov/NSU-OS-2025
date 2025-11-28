#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include <fnmatch.h>
#include <stdlib.h>

int found = 0;

char* join_path(const char *dir, const char *name) {
    size_t dir_len = strlen(dir);
    size_t name_len = strlen(name);
    size_t total_len = dir_len + name_len + 2;

    char *path = malloc(total_len);
    if (!path) {
        perror("malloc");
        return NULL;
    }

    if (dir_len == 1 && dir[0] == '/') {
        snprintf(path, total_len, "/%s", name);
    } else {
        snprintf(path, total_len, "%s/%s", dir, name);
    }

    return path;
}

void walk_dir(const char *base, char **components, int comp_count, int comp_index)
{
    if (comp_index >= comp_count) {
        return;
    }

    DIR *dir = opendir(base);
    if (!dir) {
        perror(base);
        return;
    }

    struct dirent *entry;
    while (1) {
        errno = 0;
        entry = readdir(dir);

        if (entry == NULL) {
            if (errno != 0) {
                perror(base);
            }
            break;
        }

        const char *name = entry->d_name;

        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0 && strcmp(components[comp_index], "..") != 0) {
            continue;
        }

        char *fullpath = join_path(base, name);
        if (!fullpath) {
            continue;
        }

        struct stat st;
        if (stat(fullpath, &st) != 0) {
            perror(fullpath);
            free(fullpath);
            continue;
        }

        if (fnmatch(components[comp_index], entry->d_name, 0) == 0) {
            if (comp_index == comp_count - 1) {
                printf("%s\n", fullpath);
                found = 1;
            } else if (S_ISDIR(st.st_mode)) {
                walk_dir(fullpath, components, comp_count, comp_index + 1);
            }
        }

        free(fullpath);
    }

    if (closedir(dir) != 0) {
        perror(base);
    }
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s '<pattern>'\n", argv[0]);
        return 1;
    }

    char *pattern = argv[1];
    char **components = NULL;
    int comp_count = 0;
    size_t capacity = 0;

    char *pattern_copy = strdup(pattern);
    if (!pattern_copy) {
        perror("strdup");
        return 1;
    }

    char *token = strtok(pattern_copy, "/");
    while (token) {
        if (comp_count >= capacity) {
            capacity = capacity ? capacity * 2 : 16;
            char **temp = realloc(components, capacity * sizeof(char *));
            if (!temp) {
                perror("realloc");
                break;
            }
            components = temp;
        }

        components[comp_count] = strdup(token);
        if (!components[comp_count]) {
            perror("strdup");
            break;
        }

        comp_count++;
        token = strtok(NULL, "/");
    }

    free(pattern_copy);

    char *startdir;
    if (pattern[0] == '/') {
        startdir = strdup("/");
    } else {
        startdir = strdup(".");
    }

    if (!startdir) {
        perror("strdup");
        return 1;
    }

    walk_dir(startdir, components, comp_count, 0);

    free(startdir);

    for (int i = 0; i < comp_count; i++) {
        free(components[i]);
    }
    free(components);

    if (!found) {
        printf("Your pattern: '%s'\n", argv[1]);
    }

    return 0;
}
