#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

int main() {
    // Печатаем реальный и эффективный идентификаторы пользователя
    printf("Real UID: %d\n", getuid());
    printf("Effective UID: %d\n", geteuid());
    
    // Открываем файл для проверки доступа
    FILE *file1 = fopen("file", "r");
    if (file1 == NULL) {
        perror("Error opening file");
    }

    // Устанавливаем эффективный UID, чтобы совпадал с реальным
    if (setuid(getuid()) == -1) {
        perror("setuid failed");
        fclose(file1); // Закрываем файл, если он был открыт до ошибки
        exit(1);
    }

    // Печатаем их снова после установки
    printf("After setuid:\n");
    printf("Real UID: %d\n", getuid());
    printf("Effective UID: %d\n", geteuid());

    // Открываем файл для проверки доступа
    FILE *file2 = fopen("file", "r");
    if (file2 == NULL) {
        perror("Error opening file");
        fclose(file1); // Закрываем первый файл
        return 1;
    }

    // Закрываем файлы, если они были успешно открыты
    fclose(file1);
    fclose(file2);
    printf("File opened and closed successfully.\n");

    return 0;
}
