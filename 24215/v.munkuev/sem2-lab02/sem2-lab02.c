#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

void* func(void* arg){
    int *arg_value = (int *)arg;
    for (int i = 0; i < *arg_value; i++){
        printf("%d text text\n", i);
    }

    return NULL;
}

int main(){
    int arg = 10;
    pthread_t TID;

    int res = pthread_create(&TID, NULL, func, &arg);
    if (res){
        fprintf(stderr, "pthread error\n");
        return 1;
    }
    func(&arg); 
    
    pthread_join(TID, NULL);
    printf("parent:\n");

    return 0;
}