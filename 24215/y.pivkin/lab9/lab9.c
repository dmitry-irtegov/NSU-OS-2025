#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

int main(){
    pid_t pid = fork();
    /*
        Процесс-родитель получает идентификатор (PID) потомка.
        Если это значение будет отрицательным, следовательно при порождении процесса произошла ошибка.
        Процесс-потомок получает в качестве кода возврата значение 0, если вызов fork() оказался успешным.
    */

    if (pid < 0) //ошибка
        perror("fork");
    else if (pid == 0) { //потомок
        printf("Child process started.\n");
        execl("/bin/cat", "cat", "TwoColors.txt", NULL);
        //printf("Child process closing...\n"); - не выведется
    } else { //родитель
        printf("Parent process started.\n");
        waitpid(pid, NULL, 0); //Ожидание завершения потомка
        printf("Parent process closing...\n");
    }

    return 0;
}
