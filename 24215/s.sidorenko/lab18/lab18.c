#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <string.h>
#include <time.h>
#include <libgen.h>
#include <errno.h>

static char filetype(mode_t m) {
    if (S_ISDIR(m))  return 'd';
    if (S_ISREG(m))  return '-';
    return '?';
}

static void mode_to_str(mode_t m, char out[11]) {
    out[0] = filetype(m);

    out[1] = (m & S_IRUSR) ? 'r' : '-';
    out[2] = (m & S_IWUSR) ? 'w' : '-';
    out[3] = (m & S_IXUSR) ? 'x' : '-';

    out[4] = (m & S_IRGRP) ? 'r' : '-';
    out[5] = (m & S_IWGRP) ? 'w' : '-';
    out[6] = (m & S_IXGRP) ? 'x' : '-';

    out[7] = (m & S_IROTH) ? 'r' : '-';
    out[8] = (m & S_IWOTH) ? 'w' : '-';
    out[9] = (m & S_IXOTH) ? 'x' : '-';

    out[10] = '\0';
}

static const char *just_name(const char *path) {
    if (!path) {
        return NULL;
    }
    const char *end = path;

    while (*end) {
        end++;
    }
    while ((end > path) && *(end - 1) == '/') {
        --end;
    }

    const char *slash = path;
    for (const char *p = path; p < end; ++p) {
        if (*p == '/')
            slash = p + 1;
    }
    char *copy = strdup(slash);
    if (copy) {
        return copy;
    } else {
        return path;
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Add %s filename\n", argv[0]);
        return 1;
    }

    for (int i = 1; i < argc; i++) {
        const char *path = argv[i];
        struct stat st;

        if (lstat(path, &st) < 0) {
            perror(path);
            continue;
        }

        char modes[11];
        mode_to_str(st.st_mode, modes);

        struct passwd *pw = getpwuid(st.st_uid);
        struct group  *gr = getgrgid(st.st_gid);
        const char *owner;
        const char *group;

        if (pw != NULL) {
            owner = pw->pw_name;
        } else {
            owner = "Unknown";
        }
        if (gr != NULL) {
            group = gr->gr_name;
        } else {
            group = "Unknown";
        }

        char date[32];
        struct tm *mt = localtime(&st.st_mtime);
        strftime(date, sizeof(date), "%b %d %H:%M", mt);

        const char *name = just_name(path);
        char sizebuf[32];
        const char *size_str = "";

    if (S_ISREG(st.st_mode)) {
        snprintf(sizebuf, sizeof(sizebuf), "%ld", (long)st.st_size);
        size_str = sizebuf;
    }

        printf("%-10s "            
               "%3ld "
               "%-8s %-8s "
               "%8s " 
               "%12s " 
               "%s\n",
               modes, (long)st.st_nlink, owner, group, size_str, date, name);

        free((void*)name);
    }

    return 0;
}