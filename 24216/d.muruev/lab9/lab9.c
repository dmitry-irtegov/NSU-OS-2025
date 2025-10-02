#include <stdio.h>
#include <stdlib.h>
#include <string.h> // для strcmp
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

/**
 * @brief Основная функция программы.
 * @param argc Количество аргументов командной строки.
 * @param argv Массив аргументов командной строки.
 * argv[1] - режим ('wait' или 'nowait').
 * argv[2] - имя файла для cat.
 * @return int Код завершения.
 */
int main(int argc, char *argv[])
{
    // Проверяем, что передано правильное количество аргументов
    if (argc != 2)
    {
        fprintf(stderr, "Использование: %s <wait|nowait> <имя_файла>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Проверяем корректность первого аргумента (режима)
    if (strcmp(argv[1], "wait") != 0 && strcmp(argv[1], "nowait") != 0)
    {
        fprintf(stderr, "Ошибка: Неверный режим '%s'. Используйте 'wait' или 'nowait'.\n", argv[1]);
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();

    switch (pid)
    {
    // --- Обработка ошибки ---
    case -1:
        perror("Ошибка fork");
        exit(EXIT_FAILURE);

    // --- Код дочернего процесса ---
    case 0:
        printf("--- [ДОЧЕРНИЙ ПРОЦЕСС]: Запуск cat %s ---\n\n", argv[2]);
        // Заменяем процесс на 'cat'. Передаем имя файла из аргументов.
        execlp("cat", "cat", argv[2], NULL);

        // Если execlp вернул управление, значит произошла ошибка
        perror("Ошибка execlp");
        exit(EXIT_FAILURE); // Выход из дочернего процесса с ошибкой

    // --- Код родительского процесса ---
    default:
        printf("[РОДИТЕЛЬСКИЙ ПРОЦЕСС]: Мой PID %d, PID дочернего процесса %d.\n", getpid(), pid);

        // Часть 1: Режим "nowait"
        if (strcmp(argv[1], "nowait") == 0)
        {
            printf("[РОДИТЕЛЬСКИЙ ПРОЦЕСС]: Я не жду завершения дочернего процесса.\n");
            // Родитель просто продолжает и завершает работу
        }
        // Часть 2: Режим "wait"
        else
        { // strcmp(argv[1], "wait") == 0
            printf("[РОДИТЕЛЬСКИЙ ПРОЦЕСС]: Жду завершения дочернего процесса...\n");

            // Ожидаем завершения любого дочернего процесса
            if (wait(NULL) == -1)
            {
                perror("Ошибка wait");
                exit(EXIT_FAILURE);
            }

            // Эта строка гарантированно выведется ПОСЛЕ завершения дочернего процесса
            printf("\n[РОДИТЕЛЬСКИЙ ПРОЦЕСС]: Дочерний процесс завершился. Мой финальный вывод.\n");
        }
        break;
    }

    exit(EXIT_SUCCESS);
}