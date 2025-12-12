#include <stdint.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <inttypes.h>
#include <pwd.h>
#include <grp.h>
#include <libgen.h>
#include <stdio.h>
#include <time.h>

#define type_length(t) (sizeof(t) * 3 + 2)

int main(int argc, char *argv[]) {
	struct stat statbuf;
	char mode[11];
	mode[10] = '\0';
	char uid_buffer[type_length(uintmax_t)];
	char gid_buffer[type_length(uintmax_t)];

	for (int i = 1; i < argc; i++) {
		if (lstat(argv[i], &statbuf) == -1) {
			perror(argv[i]);
			continue;
		}

		mode[0] = S_ISREG(statbuf.st_mode) ? '-' 
				: (S_ISDIR(statbuf.st_mode) ? 'd' : '?');
		mode[1] = statbuf.st_mode & S_IRUSR ? 'r' : '-';
		mode[2] = statbuf.st_mode & S_IWUSR ? 'w' : '-';
		mode[3] = statbuf.st_mode & S_IXUSR ? 'x' : '-';
		mode[4] = statbuf.st_mode & S_IRGRP ? 'r' : '-';
		mode[5] = statbuf.st_mode & S_IWGRP ? 'w' : '-';
		mode[6] = statbuf.st_mode & S_IXGRP ? 'x' : '-';
		mode[7] = statbuf.st_mode & S_IROTH ? 'r' : '-';
		mode[8] = statbuf.st_mode & S_IWOTH ? 'w' : '-';
		mode[9] = statbuf.st_mode & S_IXOTH ? 'x' : '-';

		struct passwd *pwown = getpwuid(statbuf.st_uid);
		char *owner;
		if (pwown == NULL) {
			snprintf(uid_buffer, type_length(uintmax_t), "%ju", (uintmax_t) statbuf.st_uid);
			owner = uid_buffer;
		} else {
			owner = pwown->pw_name;
		}

		struct group *grp = getgrgid(statbuf.st_gid);
		char *group;
		if (grp == NULL) {
			snprintf(gid_buffer, type_length(uintmax_t), "%ju", (uintmax_t) statbuf.st_gid);
			group = gid_buffer;
		} else {
			group = grp->gr_name;
		}

		char *mtime = ctime(&statbuf.st_mtime);
		char *name = basename(argv[i]);

		if (mode[0] == '-') {
			printf("%s  %-4ju  %-10s  %-10s  %-10jd  %.24s  %s\n",
					mode, (uintmax_t) statbuf.st_nlink,
					owner, group,
					(intmax_t) statbuf.st_size, mtime, name
			);
		} else {
			printf("%s  %-4ju  %-10s  %-10s  %10s  %.24s  %s\n",
					mode, (uintmax_t) statbuf.st_nlink,
					owner, group,
					"", mtime, name
			);
		}
	}
	exit(0);
}
