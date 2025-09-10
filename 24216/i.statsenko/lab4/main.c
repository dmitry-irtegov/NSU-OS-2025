#include <stdio.h>
#include "list.h"

char buffer[4096];
int main(void)
{
    List list = listInit();
    while (1) {
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            perror("Ошибка чтения строк");
            break;
        }
        if (buffer[0] == '.') {
            listPrint(&list);
            listDelete(&list);
            break;
        }
        if (listAppend(&list, buffer) == 0) {
            perror("Неудалось сохранить строку в памяти");
            break;
        }
    }
    return 0;
}