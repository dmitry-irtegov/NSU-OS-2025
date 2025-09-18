#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main() {
    FILE *fp; static char response;
    if((fp = fopen("answer","a+")) == NULL){
        fprintf(stderr,"Cannot open answer\n");
        exit(1);
    }
    printf("When opening a file,\n");
    printf("which id is checked?\n");
    printf(" (a)real (b) effective\n");
    while( response != 'a' && response != 'b'){
        printf("Answer(a/b)? ");
        scanf("%c%*c", &response);
    }
    printf("%5ld, %5ld\n", getuid(), geteuid());
    fprintf(fp,"%5ld:%c\n", getuid(), response);
    fclose(fp);
}