#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

void try_open(int file)
{
    if (file == NULL) {
        perror("Error opening file");
    }
    else
    {
        fclose(file);
        printf("File opened and closed successfully.\n");
    }

}

void get_id()
{
    printf("Real UID: %d\n", getuid());
    printf("Effective UID: %d\n", geteuid());
}
int main() {

    get_id();
    // Открываем файл для проверки доступа
    FILE *file1 = fopen("file", "r");
    try_open(file1);
    // Устанавливаем эффективный UID, чтобы совпадал с реальным
    if (setuid(getuid()) == -1) {
        perror("setuid failed");
        exit(1);
    }

    printf("After setuid:\n");
    get_id();
    
    // Открываем файл для проверки доступа
    FILE *file2 = fopen("file", "r");
    try_open(file2);
}
