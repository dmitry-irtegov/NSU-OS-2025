#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>

int main() {
    pid_t pid = fork();
    
    if (pid == 0) {
        execlp("cat", "cat", "./text.txt", NULL);
        
        perror("exec failed");
        exit(1);
        
    } else if (pid > 0) {
        printf("Hello!\n");

        int status;
        waitpid(pid, &status, 0); 

        printf("\n");
        
        printf("Родитель: ЭТА СТРОКА ВЫВЕДЕНА ПОСЛЕ ЗАВЕРШЕНИЯ ПОДПРОЦЕССА\n");
        
    } else {
        perror("fork failed");
        return 1;
    }
    
    return 0;
}