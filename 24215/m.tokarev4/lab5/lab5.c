#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>

#define LINES 1024
#define LINE_LEN 1024
typedef struct {
	off_t off;
	size_t len;
} Lineinfo;

int main(int argc, char* argv[]) {
	if (argc != 2) {
		fprintf(stderr, "Usage like ./textanal text.txt\n");
		exit(EXIT_FAILURE);
	}

	int fd = open(argv[1], O_RDONLY);
	Lineinfo info[LINES];
	int cntlines = 0;
	off_t offnow = 0;
	size_t lennow = 0;
	int byte = 0;
	char buf[1024];

	while ((byte = read(fd, buf, sizeof(buf))) > 0) {
		for (int i = 0; i < byte; i++) {
			if (buf[i] == '\n') {
				info[cntlines].off = offnow - lennow;
				info[cntlines++].len = lennow;

				if (cntlines >= LINES) {
					fprintf(stderr, "Too many lines\n");
					close(fd);
					exit(EXIT_FAILURE);
				}
				lennow = 0;
			}
			else {
				lennow++;
			}
			offnow++;
		}
	}
	if (lennow > 0) {
		info[cntlines].off = offnow - lennow;
		info[cntlines++].len = lennow;
	}
	printf("line number : offset : lenght\n");
	for (int i = 0; i < cntlines; i++) {
		printf("%d %d %d\n", i + 1, info[i].off, info[i].len);
	}
	while (1) {
		int numberofline;
		char linebuf[LINE_LEN];

		printf("Enter line number or 0 to exit\n");
		
		scanf("%d", &numberofline);
		if (numberofline > cntlines || numberofline < 0) {
			printf("invalid number\n\n");
			continue;
		}

		if (numberofline == 0) {
			break;
		}

		if (lseek(fd, info[numberofline - 1].off, SEEK_SET) == -1) {
			perror("lseek failed");
			continue;
		}
		int linelen = info[numberofline - 1].len;
		if (linelen > LINE_LEN) {
			linelen = LINE_LEN - 1;
		}
		byte = read(fd, linebuf, linelen);
		if (byte == -1) {
			perror("read failed");
			continue;
		}
		linebuf[byte] = '\0';
		printf("Line: %d\n", numberofline);
		printf("%s\n\n", linebuf);
	}

	close(fd);
	return 0;
}
