#include <stdio.h>
#include <string.h>
#include <dirent.h>

int validate_pattern(char *p)
{
    while (*p) {
        if (*p == '/') {
            return 1;
        }
        p++;
    }
    return 0;
}

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
        else if (*p == '?' && *n != '/') {
            p++;
            n++;
        } 
        else if (*p == *n && *n != '/') {
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


int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s '<pattern>'\n", argv[0]);
        return 1;
    }

    char *pattern = argv[1];

    if (validate_pattern(pattern)) {
        fprintf(stderr, "Error: '/' is not allowed in pattern\n");
        return 1;
    }

    DIR *dir = opendir(".");
    if (!dir) {
        perror("opendir");
        return 1;
    }

    struct dirent *entry;
    int found = 0;

    while ((entry = readdir(dir)) != NULL) {
        char *name = entry->d_name;

        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
            continue;
        }

        if (match_pattern(pattern, name)) {
            printf("%s\n", name);
            found = 1;
        }
    }

    if (closedir(dir) != 0) {
        perror("closedir");
        return 1;
    }

    if (!found) {
        printf("%s\n", pattern);
    }

    return 0;
}
