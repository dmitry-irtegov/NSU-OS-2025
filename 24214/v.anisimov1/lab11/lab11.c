#include <stdio.h> 
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#define SPLEN 14
#define SPATH ":/bin:usr/bin"

extern char **environ; 

int execvpe(const char *file, char *const argv[], char *envp[]) {

	if (file == NULL || argv == NULL || envp == NULL)
		return -1; 

	// случай когда передан полный путь к файлу 
	if (strchr(file, (int) '/') != NULL) {
		return execve(file, argv, envp);
	}

	// если надо искать через PATH
	// крузом в dirs наши директории 
	char **env = environ; 
	environ = envp; 
	char *dirs = getenv("PATH");
	environ = env; 
	char stdPaths[SPLEN + 1] = {};  
	if (dirs == NULL) {
		strcpy(stdPaths, SPATH);
		dirs = stdPaths;
	} 

	//смотрим по всем директориям, и пробуем запустить execve
	size_t fpPos = 0;  
	char fullPath[strlen(dirs) + strlen(file) + 10]; // +3: нулевой символ + символ точки (текущая директория) + слеш перед именем файла
	int execveCode = 0; 
	for (size_t i = 0; dirs[i] != '\0'; ++i) {
		// дошли до конца директории 
		if (dirs[i] == ':') {

			// если попался пустой, значит это - текущая директория
			if (!fpPos) {
				fullPath[fpPos] = '.'; 
				fpPos++;  
			} 

			if (fullPath[fpPos - 1] != '/') {
				fullPath[fpPos] = '/';
				fpPos++; 
			}
			// аккуратно помещаем имя файла в путь
			for (size_t j = 0; file[j] != '\0'; ++j) {
				fullPath[fpPos] = file[j];
				fpPos++; 
			}

			fullPath[fpPos] = '\0'; 
			fpPos = 0; 

			execveCode = execve(fullPath, argv, envp);  
			if (execveCode == -1) 
				continue; 

		// если мы всё ещё считываем путь директории 
		} else {
			fullPath[fpPos] = dirs[i]; 
			fpPos++; 
		}
	}

	// если попался пустой, значит это - текущая директория
	if (!fpPos) {
		fullPath[fpPos] = '.'; 
		fpPos++;  
	}

	if (fullPath[fpPos - 1] != '/') {
		fullPath[fpPos] = '/';
		fpPos++; 
	}

	// аккуратно помещаем имя файла в путь
	for (size_t j = 0; file[j] != '\0'; ++j) {
		fullPath[fpPos] = file[j];
		fpPos++; 
	}

	fullPath[fpPos] = '\0';

	return execve(fullPath, argv, envp);

}

int main() {
	const char* file = "./time"; 
	char *const argv[2] = {"time", NULL};
	char *envp[3] = {"TZ=America/Los_Angeles", NULL}; 
	int execvpeCode = execvpe(file, argv, envp);
	if (execvpeCode == -1)
		printf("execvpe running finished unsuccessfully\n");
	return 0; 
}