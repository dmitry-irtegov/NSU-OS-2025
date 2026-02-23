#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <stdbool.h>

static bool match_pattern(const char *pattern, const char *str) {
    if (*pattern == '\0' && *str == '\0') {
        return true;
    }
    
    if (*pattern == '*') {
        if (*(pattern + 1) == '\0') {
            return true;
        }
        
        while (*str != '\0') {
            if (match_pattern(pattern + 1, str)) {
                return true;
            }
            str++;
        }
        
        return match_pattern(pattern + 1, str);
    }
    
    if (*pattern == '?' && *str != '\0') {
        return match_pattern(pattern + 1, str + 1);
    }
    
    if (*pattern == *str) {
        return match_pattern(pattern + 1, str + 1);
    }
    
    return false;
}

int main(void) {
    char pattern[256];
    DIR *dir;
    struct dirent *entry;
    int match_count = 0;
    
    printf("Enter file pattern: ");
    if (fgets(pattern, sizeof(pattern), stdin) == NULL) {
        fprintf(stderr, "Error reading pattern\n");
        return 1;
    }
    
    size_t len = strlen(pattern);
    if (len > 0 && pattern[len - 1] == '\n') {
        pattern[len - 1] = '\0';
    }
    
    if (strchr(pattern, '/') != NULL) {
        fprintf(stderr, "Error: '/' is not allowed in pattern\n");
        return 1;
    }
    
    dir = opendir(".");
    if (dir == NULL) {
        perror("opendir");
        return 1;
    }
    
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        if (match_pattern(pattern, entry->d_name)) {
            printf("%s\n", entry->d_name);
            match_count++;
        }
    }
    
    if (match_count == 0) {
        printf("%s\n", pattern);
    }
    
    closedir(dir);
    return 0;
}
