#include <errno.h>
#include <grp.h>
#include <libgen.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

char get_file_type(mode_t mode) {
    if (S_ISDIR(mode)) {
        return 'd';
    } else if (S_ISREG(mode)) {
        return '-';
    }
    return '?';
}

void get_rules(mode_t mode, char* buffer) {
    buffer[0] = (mode & S_IRUSR) ? 'r' : '-';
    buffer[1] = (mode & S_IWUSR) ? 'w' : '-';
    buffer[2] = (mode & S_IXUSR) ? 'x' : '-';
    buffer[3] = (mode & S_IRGRP) ? 'r' : '-';
    buffer[4] = (mode & S_IWGRP) ? 'w' : '-';
    buffer[5] = (mode & S_IXGRP) ? 'x' : '-';
    buffer[6] = (mode & S_IROTH) ? 'r' : '-';
    buffer[7] = (mode & S_IWOTH) ? 'w' : '-';
    buffer[8] = (mode & S_IXOTH) ? 'x' : '-';
    buffer[9] = '\0';
}

char* get_name_owner(uid_t uid) {
    struct passwd* pwd = getpwuid(uid);
    return pwd ? pwd->pw_name : "unknown";
}
char* get_name_group(gid_t gid) {
    struct group* grp = getgrgid(gid);
    return grp ? grp->gr_name : "unknown";
}

int get_size_file(struct stat* sb) {
    return S_ISREG(sb->st_mode) ? sb->st_size : -1;
}

void get_date_modification(time_t mtime, char* buffer, size_t bufsize) {
    struct tm* tm_info = localtime(&mtime);
    strftime(buffer, bufsize, "%Y-%m-%d %H:%M", tm_info);
}

char* get_file_name(const char* path) {
    char* file_name = strchr(path, '/');
    return file_name ? file_name + 1 : (char*)path;
}

void print_file_info(const char* path) {
    struct stat sb;
    if (lstat(path, &sb) == -1) {
        perror("lstat");
        return;
    }

    char file_type = get_file_type(sb.st_mode);

    char rules[10];
    get_rules(sb.st_mode, rules);

    nlink_t link_count = sb.st_nlink;

    char* owner_name = get_name_owner(sb.st_uid);
    char* group_name = get_name_group(sb.st_gid);

    char file_size_str[32] = "";
    int file_size = get_size_file(&sb);
    if (file_size != -1) {
        snprintf(file_size_str, sizeof(file_size_str), "%d", file_size);
    }

    char date_mod[20];
    get_date_modification(sb.st_mtime, date_mod, sizeof(date_mod));

    char* file_name = get_file_name(path);

    printf("%c%s %lu %s %s %8s %s %s\n", file_type, rules, link_count,
           owner_name, group_name, file_size_str, date_mod, file_name);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <file1> <file2> ...\n", argv[0]);
        return EXIT_FAILURE;
    }
    for (int i = 1; i < argc; i++) {
        print_file_info(argv[i]);
    }

    return 0;
}