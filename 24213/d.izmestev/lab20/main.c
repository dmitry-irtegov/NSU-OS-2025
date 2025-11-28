#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <limits.h>

int found = 0;

int match_pattern(char *pattern, char *name)
{
    char *p = pattern;
    char *n = name;
    char *star = NULL;
    char *backup = NULL;

    while (*n) {
        if (*p == '*') {
            star = p++;
            backup = n;
        }
        else if (*p == '?') {
            p++;
            n++;
        }
        else if (*p == *n) {
            p++;
            n++;
        }
        else if (star) {
            p = star + 1;
            n = ++backup;
        }
        else {
            return 0;
        }
    }

    while (*p == '*') {
        p++;
    }

    return *p == '\0';
}


void walk_dir(char *base, char *pattern)
{
    DIR *dir = opendir(base);
    if (!dir) {
        perror(base);
        return;
    }

    struct dirent *entry;
    
    while ((entry = readdir(dir)) != NULL) {
        char *name = entry->d_name;

        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
            continue;
        }

        char fullpath[PATH_MAX];

        if (strcmp(base, ".") == 0) {
            snprintf(fullpath, sizeof(fullpath), "%s", name);
        } else {
            snprintf(fullpath, sizeof(fullpath), "%s/%s", base, name);
        }

        struct stat st;
        if (stat(fullpath, &st) != 0) {
            perror(fullpath);
            continue;
        }

        if (S_ISREG(st.st_mode) && match_pattern(pattern, fullpath)) {
            printf("%s\n", fullpath);
            found = 1;
        }

        if (S_ISDIR(st.st_mode)) {
            walk_dir(fullpath, pattern);
        }
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

    walk_dir(".", argv[1]);

    if (!found) {
        printf("%s\n", argv[1]);
    }

    return 0;
}
