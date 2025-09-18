#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main() {
    FILE *fp;
    if((fp = fopen("answer","a+")) == NULL){
        fprintf(stderr,"Cannot open answer\n");
        perror("Error");
        exit(1);
    }
    fclose(fp);

    printf("%5ld, %5ld\n", getuid(), geteuid());
    setuid(getuid());
    
    if((fp = fopen("answer","a+")) == NULL){
        fprintf(stderr,"Cannot open answer\n");
        perror("Error");
        exit(1);
    }
    fclose(fp);
    
    printf("%5ld, %5ld\n", getuid(), geteuid());
    
    return 0;
}