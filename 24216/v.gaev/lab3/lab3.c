#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

int main() {
    // Печатаем реальный и эффективный идентификаторы пользователя
    printf("Real UID: %d\n", getuid());
    printf("Effective UID: %d\n", geteuid());
    char try_open = 0; 
    // Открываем файл для проверки доступа
    FILE *file1 = fopen("file", "r");
    if (file1 == NULL) {
        perror("Error opening file");
    }

    // Устанавливаем эффективный UID, чтобы совпадал с реальным
    if (setuid(getuid()) == -1) {
        perror("setuid failed");
        exit(1);
    }

    // Печатаем их снова после установки
    printf("After setuid:\n");
    printf("Real UID: %d\n", getuid());
    printf("Effective UID: %d\n", geteuid());

    // Открываем файл для проверки доступа
    FILE *file2 = fopen("file", "r");
    if (file2 == NULL) {
        try_open = 1;
        perror("Error opening file");
    }

    // Закрываем файл, если он был успешно открыт
    fclose(file1);
    fclose(file2);
    if (try_open == 0){
        printf("File opened and closed successfully.\n");
    }
    return 0;
}
