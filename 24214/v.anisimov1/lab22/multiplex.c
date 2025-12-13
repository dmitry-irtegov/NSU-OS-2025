#include <unistd.h> 
#include <stdio.h>
#include <stdlib.h>  
#include <signal.h>
#include <strings.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h> 

#define TIME_OUT 10
#define BUFFSIZE 256

typedef struct fileInfo { 
    int fd; 
    int isEOF;  
    int isOpen; 
    char *buffer; 
    size_t buffSize; 
    size_t buffCapacity; 
} fileInfo; 

int fileHandle(fileInfo *fInf, const char *fileName) {
    int fd = open(fileName, O_RDONLY); 
    if (fd == -1) {
        perror("APP: [open] error occured");
        return -1; 
    } 
    fInf->fd = fd;
    fInf->isEOF = 0;
    fInf->isOpen = 1;
    fInf->buffSize = 0; 
    fInf->buffCapacity = 128; 
    fInf->buffer = (char*) malloc (sizeof(char) * 128); 
    if (fInf->buffer == NULL) {
        perror("APP: [malloc] error");
        return -1; 
    }
    return 0;  
}

int printStringFromFile(fileInfo *fInf) { // распечатать строку, прочитанную с файла 
    size_t i = 0; 
    size_t lastSize = 0;
    size_t strSize = 0; 
    while (i < fInf->buffSize) {
        if ((fInf->buffer[i] == '\n' || fInf->buffer[i] == '\0')) {
            lastSize = fInf->buffSize; // размер до чистки буфера
            fInf->buffer[i] = '\0';
            if (strSize > 0) {
                printf("STRING: %s\n", fInf->buffer);
            } else if (strSize == 0 && !(fInf->isEOF)) {
                printf("STRING: --EMPTY STRING--\n");
            }
            fInf->buffSize -= strSize + 1; 
            i++;
            memmove(fInf->buffer, fInf->buffer + i, lastSize - i);
            return 1; // нашли и вывели одну строку, отчистили от неё буфер
        } else {
            strSize++; 
            i++;  
        } 
    }
    return 0; // в буфере файла нету строк 
}

int putChtrsIntoBuffer(fileInfo *fInf, char* charArr, size_t amount) {
    if (fInf->buffSize + amount > fInf->buffCapacity) {
        char *newBuff = (char*) realloc (fInf->buffer, (fInf->buffSize + amount) * 2);
        if (newBuff == NULL) {
            perror("APP: [realloc] - not enough memory");
            return -1; 
        }
        fInf->buffer = newBuff;
        fInf->buffCapacity = (fInf->buffSize + amount) * 2; 
    }
    memcpy(fInf->buffer + fInf->buffSize, charArr, amount);
    fInf->buffSize += amount; 
    return 0;  
}

void cleanFileInfo(fileInfo *fInf) {
    if (fInf->isOpen) {
        free(fInf->buffer); 
        close(fInf->fd);
        fInf->isOpen = 0; 
    }
}

void totalClean(fileInfo *arr, size_t arrSize) {
    for (size_t i = 0; i < arrSize; ++i) {
        if (arr[i].isOpen) {
            free(arr[i].buffer); 
            close(arr[i].fd);
            arr[i].isOpen = 0;
        }
    }
}

int alrmFlag = 0; 

void catchALRM(int sig) {
    if (sig == SIGALRM)
        alrmFlag = 1;
}

int main(int argc, char** argv) {

    if (argc < 2) {
        printf("APP: there are no arguments\n");
        return 0; 
    }
    
    size_t filesAmount = (size_t) (argc) - 1; 
    printf("APP: handling arguments\n");
    fileInfo processedFiles[filesAmount]; 

    // открываем файлы
    for (size_t i = 0; i < filesAmount; ++i) {
        if (fileHandle(&processedFiles[i], argv[i + 1]) == -1) {
            for (size_t j = 0; j < i; j++) {
                free(processedFiles[j].buffer); 
                close(processedFiles[j].fd);
            }
        return 0; 
        } else {
            printf("APP: File %s opened successfully\n", argv[i + 1]); 
        }
    }

    // устанавливаем обработчик сигнала SIGALRM
    if (sigset(SIGALRM, catchALRM) == SIG_ERR) {
        perror("ERROR: [sigset]");
        for (size_t i = 0; i < (size_t) argc; ++i)
            close(processedFiles[i].fd); 
        return 0; 
    }
    
    size_t fdIdx = 0; 
    size_t closedFiles = 0; 
    ssize_t readCode = 0; 
    char buffer[BUFFSIZE]; // буфер, куда считываем данные с файлов  
    while (closedFiles < filesAmount) {
        if (!processedFiles[fdIdx].isOpen) {
            fdIdx = (fdIdx + 1) % filesAmount; // если файл закрыт - идём дальше
            continue; 
        }
        if (printStringFromFile(&processedFiles[fdIdx])) {
            fdIdx = (fdIdx + 1) % filesAmount; // если у буфере уже была строка, то мы просто выводим её
            continue; 
        } else { // если в буфере нет строк - пытаемся считать данные 
            alrmFlag = 0; 
            alarm(TIME_OUT); 
            readCode = read(processedFiles[fdIdx].fd, buffer, BUFFSIZE);
            alarm(0); 
            if (alrmFlag) {
                printf("APP: TIMEOUT!\n");
                alrmFlag = 0; 
                fdIdx = (fdIdx + 1) % filesAmount;
                continue;
            } else { 
                switch (readCode) {
                    case -1: 
                        perror("APP: [read] non-signal error\n");
                        totalClean(processedFiles, filesAmount);
                        return 0;  
                    case 0: 
                        buffer[0] = '\0';
                        if (putChtrsIntoBuffer(&processedFiles[fdIdx], buffer, 1) == -1) { // обозначаем "конец" данных
                            totalClean(processedFiles, filesAmount);
                            return 0; 
                        }
                        processedFiles[fdIdx].isEOF = 1; 
                        int psf = 0; 
                        do {
                            psf = printStringFromFile(&processedFiles[fdIdx]);
                        } while (psf); // печатаем все оставшиеся строки в из буфера, если есть 
                        cleanFileInfo(&processedFiles[fdIdx]);
                        closedFiles++; 
                        fdIdx = (fdIdx + 1) % filesAmount;
                        break; 
                    default:
                        if (putChtrsIntoBuffer(&processedFiles[fdIdx], buffer, (size_t) readCode) == -1) { // если строки в буфере для файла не было, то продолжаем читать с этого файла
                            totalClean(processedFiles, filesAmount); 
                            return 0; 
                        } 
                }
            }
        }
    }
    totalClean(processedFiles, filesAmount);
    return 0; 
}
