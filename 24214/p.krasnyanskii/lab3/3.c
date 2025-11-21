#include <stdio.h>
#include <unistd.h>

void getIds(){
    printf("Your real id: %d\n", getuid());
    printf("Your effective id: %d\n", geteuid());
}

void openFile(FILE *in){
        in = fopen("file.txt", "r");
        if (in == NULL)
                perror("You haven't enough permissions to open this file\n");
        else printf("File opened succesfully\n");
        fclose(in);
}

int main(){
        printf("\n");
        FILE* in;
        
        getIds();
        openFile(in);

        setuid(getuid());

        getIds();
        openFile(in);

        return 0;
}

