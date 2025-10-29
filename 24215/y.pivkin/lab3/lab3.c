#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>

//verify 

void twoSteps(){
    uid_t ruid = getuid(); //реальный
    uid_t euid = geteuid(); //эффективный

    //1
    printf("Real User ID = %d\n", ruid);
    printf("Effective User ID = %d\n", euid);

    //2
    FILE *f = fopen("test", "r+");
    if (!f) {
        perror("fopen");
        return;
    }
    printf("File successfully opened.\n");

    int not_closed = fclose(f);
    if (not_closed) {
        perror("fclose");
        return;
    }
    printf("File successfully closed.\n");
}



int main(){
    // 1 и 2 шаги
    twoSteps();

    //3
    uid_t ruid = getuid();
    int not_changed = seteuid(ruid);
    if (not_changed) {
        perror("setuid");
        return 0;
    }
    printf("====== UID changed! ======\n");

    //4
    twoSteps();

    return 0;
}
