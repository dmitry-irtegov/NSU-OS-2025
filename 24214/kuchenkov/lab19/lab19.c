#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <fnmatch.h>

int main() {
    char str[BUFSIZ];

    fgets(str, BUFSIZ, stdin);
    str[strcspn(str, "\n")] = 0;

    DIR* dir = opendir(".");
    struct dirent* entry;
    int found = 0;

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        if (fnmatch(str, entry->d_name, 0) == 0) {
            printf("Found file: %s\n", entry->d_name);
            found = 1;
        }
    }
    
    if (found == 0) {
        printf("%s\n", str);
    }

    closedir(dir);
    return 0;

}