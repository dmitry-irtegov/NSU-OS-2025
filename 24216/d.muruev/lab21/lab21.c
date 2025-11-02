#include <stdio.h>
#include <stdlib.h>
#include <signal.h> 
#include <unistd.h> 


volatile sig_atomic_t beep_count = 0;
volatile sig_atomic_t quit_flag = 0;

/**
 * @brief Обработчик сигнала SIGINT (Ctrl+C).
 * * Увеличивает счетчик и издает звуковой сигнал.
 */
void handle_sigint(int sig) {
    beep_count++;
    
    // Используем write() вместо printf(), так как printf()
    // не является "async-signal-safe" (небезопасен для вызова
    // из обработчика сигнала).
    // '\a' - это "alert" (bell) символ.
    write(STDOUT_FILENO, "\a", 1);
    
    // Примечание: на старых системах (System V) обработчик 
    // нужно было переустанавливать. В POSIX (Linux, macOS)
    // этого делать не нужно.
    // signal(SIGINT, handle_sigint); 
}

/**
 * @brief Обработчик сигнала SIGQUIT (Ctrl+\).
 * * Устанавливает флаг для завершения основного цикла.
 * Мы не выводим сообщение здесь, так как printf() небезопасен.
 */
void handle_sigquit(int sig) {
    quit_flag = 1;
}

int main() {
    // 1. Устанавливаем наши обработчики сигналов
    
    // Привязываем handle_sigint к SIGINT
    if (signal(SIGINT, handle_sigint) == SIG_ERR) {
        perror("Ошибка установки обработчика SIGINT");
        exit(1);
    }

    // Привязываем handle_sigquit к SIGQUIT
    if (signal(SIGQUIT, handle_sigquit) == SIG_ERR) {
        perror("Ошибка установки обработчика SIGQUIT");
        exit(1);
    }

    printf("Программа запущена.\n");
    printf("Нажмите Ctrl+C для звукового сигнала (SIGINT).\n");
    printf("Нажмите Ctrl+\\ для выхода и подсчета (SIGQUIT).\n");

    // 2. Бесконечный цикл
    //    pause() приостанавливает выполнение процесса, 
    //    пока не будет получен сигнал, который 
    //    либо завершит процесс, либо вызовет обработчик.
    while (quit_flag == 0) {
        pause();
    }

    // 3. Завершение
    // Сюда мы попадем, только когда quit_flag станет 1 (т.е. после SIGQUIT).
    // Теперь мы в основном потоке (не в обработчике),
    // поэтому можем безопасно использовать printf().
    
    printf("\nПолучен сигнал SIGQUIT. Завершение.\n");
    
    // (int) используется для приведения sig_atomic_t к int для printf
    printf("Всего прозвучало сигналов: %d\n", (int)beep_count);

    return 0;
}