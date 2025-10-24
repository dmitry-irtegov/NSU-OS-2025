#include <stdio.h>
#include <termios.h>
#include <unistd.h>

char getch(void) {
    struct termios oldt, newt;
    char ch;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
}

int main() {
    printf("Введите любой символ: ");
    char c = getch();
    printf("\nВы ввели: '%c'\n", c);
    return 0;
}