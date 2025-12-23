#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <libgen.h>
#include <string.h>
#include <errno.h>

void print_permissions(mode_t mode) {
    if (S_ISDIR(mode)) printf("d");
    else if (S_ISREG(mode)) printf("-");
    else printf("?");
    
    printf((mode & S_IRUSR) ? "r" : "-");
    printf((mode & S_IWUSR) ? "w" : "-");
    printf((mode & S_IXUSR) ? "x" : "-");
    printf((mode & S_IRGRP) ? "r" : "-");
    printf((mode & S_IWGRP) ? "w" : "-");
    printf((mode & S_IXGRP) ? "x" : "-");
    printf((mode & S_IROTH) ? "r" : "-");
    printf((mode & S_IWOTH) ? "w" : "-");
    printf((mode & S_IXOTH) ? "x" : "-");
    printf(" ");
}


void print_owner_group(uid_t uid, gid_t gid) {
    struct passwd *pw = getpwuid(uid);
    struct group *gr = getgrgid(gid);
    printf("%-8.8s %-8.8s ", pw ? pw->pw_name : "unknown", gr ? gr->gr_name : "unknown");
}

void print_size(mode_t mode, off_t size) {
    if (S_ISREG(mode)) printf("%8lu ", (unsigned long)size);
    else printf("         ");
}

void print_mtime(time_t mtime) {
    struct tm *tm = localtime(&mtime);
    char buf[20];
    strftime(buf, sizeof(buf), "%b %e %H:%M", tm);
    printf("%s ", buf);
}

void print_filename(const char *path) {
    printf("%s\n", basename((char *)path));
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s file1 [file2 ...]\n", argv[0]);
        return 1;
    }
    
    for (int i = 1; i < argc; i++) {
        struct stat st;
        if (lstat(argv[i], &st) == -1) {
            fprintf(stderr, "%s: %s\n", argv[i], strerror(errno));
            continue;
        }
        
        print_permissions(st.st_mode);
        printf("%4lu ", (unsigned long)st.st_nlink);
        print_owner_group(st.st_uid, st.st_gid);
        print_size(st.st_mode, st.st_size);
        print_mtime(st.st_mtime);
        print_filename(argv[i]);
    }
    return 0;
}
