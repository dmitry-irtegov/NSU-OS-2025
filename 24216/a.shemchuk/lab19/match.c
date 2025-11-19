#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>

bool match(char* ptr, char* str) {
    if (*ptr == '\0' && *str == '\0') {
        return true;
    }
    if (*ptr == '\0') {
        return false;
    }

    switch (*ptr)
    {
    case '?':
        return match(ptr + 1, str + 1);
    case '*':
        return  match(ptr + 1, str) || // * - empty sequence
            ((*str != '\0') && match(ptr, str + 1));
    default:
        return (*ptr == *str) && (*str != '\0') && match(ptr + 1, str + 1);
    }
    return false;
}

int main(int argc, char** argv) {
#ifndef TEST
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <ptr>\n", argv[0]);
        return 1;
    }
    if (strchr(argv[1], '/')) {
        fprintf(stderr, "slash is not allowed in pattern\n");
        return 1;
    }

    DIR* d = opendir("."); //cwd
    if (!d) {
        perror("opendir");
        return 1;
    }

    struct dirent* dentry;
    bool found = false;
    while ((dentry = readdir(d)) != NULL) {
        if (match(argv[1], dentry->d_name)) {
            printf("%s\n", dentry->d_name);
            found = true;
        }
    }

    if (!found) {
        printf("Matches not found, pattern: %s\n", argv[1]);
    }
    closedir(d);
    return 0;
#else // ifdef TEST
#include <assert.h>
    (void)argc;
    (void)argv;
    assert(match("", ""));
    assert(match("test", "test"));
    assert(match("hello", "hello"));
    assert(match("file.txt", "file.txt"));
    assert(!match("test", "test1"));
    assert(!match("long", "short"));

    assert(match("?", "a"));
    assert(match("?", "1"));
    assert(!match("?", ""));

    assert(match("f?le", "file"));
    assert(match("f?le", "fxle"));
    assert(match("f?le", "fale"));
    assert(!match("f?le", "fle"));
    assert(!match("f?le", "fxxle"));
    assert(match("??", "ab"));
    assert(!match("??", "a"));
    assert(match("te?t", "test"));
    assert(match("te?t", "text"));
    assert(match("te?t", "te t"));
    assert(!match("te?t", "tet"));

    assert(match("*", ""));
    assert(match("*", "a"));
    assert(match("*", "anything"));
    assert(match("*", "very long string with spaces"));
    assert(match("*.txt", "file.txt"));
    assert(match("*.txt", ".txt"));
    assert(match("*.txt", "document.txt"));
    assert(!match("*.txt", "file.jpg"));
    assert(match("file*", "file"));
    assert(match("file*", "file123"));
    assert(match("file*", "file.txt"));
    assert(!match("file*", "document"));
    assert(match("*.*", "file.txt"));
    assert(match("*.*", "a.b"));
    assert(!match("*.*", "file"));
    assert(match("a*b", "ab"));
    assert(match("a*b", "acb"));
    assert(match("a*b", "accb"));
    assert(match("a*b", "a123456789b"));
    assert(!match("a*b", "ac"));
    assert(!match("a*b", "cb"));

    assert(match("f*?e", "file"));
    assert(match("f*?e", "fake"));
    assert(match("f*?e", "fale"));
    assert(!match("f*?e", "fe"));
    assert(match("img?*.jpg", "img1.jpg"));
    assert(match("img?*.jpg", "imgA.jpg"));
    assert(match("img?*.jpg", "img123.jpg"));
    assert(!match("img?*.jpg", "img.jpg"));
    assert(match("file?.*", "file1.txt"));
    assert(match("file?.*", "fileA.jpg"));
    assert(!match("file?.*", "file.txt"));
    assert(match("a?*b", "aXb"));
    assert(match("a?*b", "aXYZb"));
    assert(!match("a?*b", "ab"));
    assert(match("a*?b", "aXb"));
    assert(match("a*?b", "aXYZb"));
    assert(!match("a*?b", "ab"));

    assert(match("a*b*c", "abc"));
    assert(match("a*b*c", "aXbYc"));
    assert(match("a*b*c", "a123b456c"));
    assert(!match("a*b*c", "ac"));
    assert(!match("a*b*c", "ab"));
    assert(match("*a*b*", "xaybz"));
    assert(match("*a*b*", "ab"));
    assert(!match("*a*b*", "ba"));
    assert(match("**", "anything"));
    assert(match("a**b", "aXb"));

    assert(match("", ""));
    assert(!match("a", ""));
    assert(!match("", "a"));
    assert(match("*", ""));
    assert(!match("?", ""));
    assert(!match("a*", ""));

    assert(match("*.c", "main.c"));
    assert(match("*.c", "library.c"));
    assert(!match("*.c", "file.h"));
    assert(match("test_*", "test_unit"));
    assert(match("test_*", "test_integration"));
    assert(!match("test_*", "unit_test"));
    assert(match("*.tar.gz", "archive.tar.gz"));
    assert(match("*.tar.gz", "backup.tar.gz"));
    assert(!match("*.tar.gz", "archive.gz"));
    printf("All tests are passed!\n");
#endif
    return 0;
}