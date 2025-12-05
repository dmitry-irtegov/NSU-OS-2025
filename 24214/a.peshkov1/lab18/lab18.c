#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void mode_to_str(mode_t m, char out[11]) {
    out[0] = S_ISDIR(m) ? 'd' : S_ISREG(m) ? '-' : '?';
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

static const char *basename_const(const char *path) {
    if (!path || !*path) return path ? path : "";
    const char *last = strrchr(path, '/');
    return last ? last + 1 : path;
}

static int print_uid_col(uid_t uid) {
    struct passwd *pw = getpwuid(uid);
    if (pw && pw->pw_name)
        return printf("%-16.16s", pw->pw_name);
    return printf("%-16lu", (unsigned long) uid);
}

static int print_gid_col(gid_t gid) {
    struct group *gr = getgrgid(gid);
    if (gr && gr->gr_name)
        return printf("%-16.16s", gr->gr_name);
    return printf("%-16u", (unsigned) gid);
}

static int print_size_col(const struct stat *st) {
    if (S_ISREG(st->st_mode))
        return printf("%16lld", (long long) st->st_size);
    return printf("%16s", "");
}

static int print_mtime_col(const struct stat *st) {
    char buf[32] = {0};
    struct tm tm;
    if (localtime_r(&st->st_mtime, &tm)) {
        if (strftime(buf, sizeof buf, "%Y-%m-%d %H:%M", &tm) > 0)
            return printf("%16s", buf);
    }

    return printf("%16s", "????????????????");
}

static void chkprintf(int rc) {
    if (rc < 0) {
        perror("printf");
        clearerr(stdout);
    }
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s file...\n", argv[0]);
        return 1;
    }

    for (int i = 1; i < argc; i++) {
        const char *path = argv[i];
        struct stat st;

        if (lstat(path, &st) == -1) {
            perror(path);
            continue;
        }

        char mstr[11];
        mode_to_str(st.st_mode, mstr);

        chkprintf(printf("%s %4lu ", mstr, (unsigned long) st.st_nlink));
        chkprintf(print_uid_col(st.st_uid));
        chkprintf(printf(" "));
        chkprintf(print_gid_col(st.st_gid));
        chkprintf(printf(" "));
        chkprintf(print_size_col(&st));
        chkprintf(printf(" "));
        chkprintf(print_mtime_col(&st));
        chkprintf(printf(" %s\n", basename_const(path)));
    }
    return 0;
}
