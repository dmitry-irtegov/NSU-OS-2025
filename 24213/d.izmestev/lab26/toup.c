#include <stdio.h>
#include <ctype.h>

#define BUF_SIZE 1024

int main()
{
    char buf[BUF_SIZE];
    size_t n = fread(buf, 1, sizeof(buf), stdin);

    while (n > 0) {
        for (size_t i = 0; i < n; i++) {
            buf[i] = toupper((unsigned char)buf[i]);
        }

        if (fwrite(buf, 1, n, stdout) != n) {
            perror("fwrite");
            return 1;
        }
        n = fread(buf, 1, sizeof(buf), stdin);
    }

    if (ferror(stdin)) {
        perror("fread");
        return 1;
    }

    return 0;
}
