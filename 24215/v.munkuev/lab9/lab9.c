#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>


#define PATH "file.txt"

int main() {
    int status;
    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        return 1;
    }
    else if(pid == 0){
        //ребенок
        printf("CHILD: ПРИВЕТ Я РЕБЕНОК");
        execl("/bin/cat", "cat", PATH, NULL);
        perror("exec error");
        return 1;
    }
    else{
        //родитель
        printf("PARENT: ЖДУ РЕБЕНКА!\n");

        if(waitpid(pid, &status, 0) == -1){
            perror("waitpid error");
            return 1;
        }

        printf("PARENT: ДОЖДАЛСЯ РЕБЕНКА!\n");
    }

    return 0;

}