#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main() {
    printf("%5ld, %5ld\n", getuid(), geteuid());
    FILE *fp;
    if((fp = fopen("answer","a+")) == NULL){
        fprintf(stderr,"Cannot open answer\n");
        perror("Error");
        exit(1);
    }
    fclose(fp);

    setuid(getuid());

    printf("%5ld, %5ld\n", getuid(), geteuid());
    if((fp = fopen("answer","a+")) == NULL){
        fprintf(stderr,"Cannot open answer\n");
        perror("Error");
        exit(1);
    }
    fclose(fp);
    
    return 0;
}