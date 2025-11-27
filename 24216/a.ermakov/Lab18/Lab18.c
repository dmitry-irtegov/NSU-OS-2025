#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <string.h>
#include <libgen.h>

#define MAX_FILES 1024

char get_filetype(mode_t mode) {
    if (S_ISDIR(mode)) return 'd';
    if (S_ISREG(mode)) return '-';
    if (S_ISLNK(mode)) return 'l';
    return '?';
}

void get_permissions(mode_t mode, char *buf) {
    buf[0] = (mode & S_IRUSR) ? 'r' : '-';
    buf[1] = (mode & S_IWUSR) ? 'w' : '-';
    buf[2] = (mode & S_IXUSR) ? 'x' : '-';
    buf[3] = (mode & S_IRGRP) ? 'r' : '-';
    buf[4] = (mode & S_IWGRP) ? 'w' : '-';
    buf[5] = (mode & S_IXGRP) ? 'x' : '-';
    buf[6] = (mode & S_IROTH) ? 'r' : '-';
    buf[7] = (mode & S_IWOTH) ? 'w' : '-';
    buf[8] = (mode & S_IXOTH) ? 'x' : '-';
    buf[9] = '\0';
}

void format_time(time_t t, char *buf, size_t sz) {
    struct tm *tm = localtime(&t);
    strftime(buf, sz, "%Y-%m-%d %H:%M", tm);
}

const char* get_owner(uid_t uid) {
    struct passwd *pw = getpwuid(uid);
    return pw ? pw->pw_name : "unknown";
}

const char* get_group(gid_t gid) {
    struct group *gr = getgrgid(gid);
    return gr ? gr->gr_name : "unknown";
}

void print_filetype(mode_t mode) {
    printf("%c", get_filetype(mode));
}

void print_permissions(mode_t mode) {
    char buf[10];
    get_permissions(mode, buf);
    printf("%s", buf);
}

void print_owner(const char *owner, int width) {
    printf(" %-*s", width, owner);
}

void print_group(const char *group, int width) {
    printf(" %-*s", width, group);
}

void print_nlink(nlink_t nlink) {
    printf(" %3u", (unsigned)nlink);
}

void print_size(off_t size, mode_t mode) {
    if (S_ISREG(mode))
        printf(" %8ld", (long)size);
    else
        printf(" %8s", "");
}

void print_time(time_t t) {
    char tbuf[64];
    format_time(t, tbuf, sizeof(tbuf));
    printf(" %s", tbuf);
}

void print_name(const char *name) {
    printf(" %s", name);
}

typedef struct {
    struct stat st;
    const char *name;
    char owner[64];
    char group[64];
} FileInfo;

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s file...\n", argv[0]);
        return 1;
    }

    FileInfo files[MAX_FILES];
    int count = 0;

    for (int i = 1; i < argc; i++) {
        if (count >= MAX_FILES) {
            fprintf(stderr, "Too many files\n");
            break;
        }

        if (lstat(argv[i], &files[count].st) < 0) {
            perror(argv[i]);
            continue;
        }

        files[count].name = basename(argv[i]);

        snprintf(files[count].owner, sizeof(files[count].owner),
                 "%s", get_owner(files[count].st.st_uid));

        snprintf(files[count].group, sizeof(files[count].group),
                 "%s", get_group(files[count].st.st_gid));

        count++;
    }

    int owner_width = 0, group_width = 0;

    for (int i = 0; i < count; i++) {
        int ow = strlen(files[i].owner);
        int gw = strlen(files[i].group);
        if (ow > owner_width) owner_width = ow;
        if (gw > group_width) group_width = gw;
    }
    
    for (int i = 0; i < count; i++) {
        print_filetype(files[i].st.st_mode);
        print_permissions(files[i].st.st_mode);

        print_nlink(files[i].st.st_nlink);

        print_owner(files[i].owner, owner_width);
        print_group(files[i].group, group_width);

        print_size(files[i].st.st_size, files[i].st.st_mode);

        print_time(files[i].st.st_mtime);
        print_name(files[i].name);

        printf("\n");
    }

    return 0;
}
