#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

int main() {
    uid_t real_uid, effective_uid;
    FILE *file;

    real_uid = getuid();
    effective_uid = geteuid();
    
    printf("1. До setuid:\n");
    printf("  Real UID: %d\n", real_uid);
    printf("  Effective UID: %d\n", effective_uid);


    file = fopen("file", "r+");
    if (file == NULL) {
        perror("2. Ошибка открытия файла");
    } else {
        printf("2. Файл успешно открыт\n");
        fclose(file);
    }

    
    if (setuid(real_uid) == -1) {
        perror("3. Ошибка setuid");
        return 1;
    }
    printf("3. UID установлены\n");


    real_uid = getuid();
    effective_uid = geteuid();
    printf("4. После setuid:\n");
    printf("  Real UID: %d\n", real_uid);
    printf("  Effective UID: %d\n", effective_uid);

    file = fopen("file", "r+");
    if (file == NULL) {
        perror("4. Ошибка открытия файла");
    } else {
        printf("4. Файл успешно открыт\n");
        fclose(file);
    }

    return 0;
}