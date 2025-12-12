COMPILER_FLAGS=-Wall -Wextra -Werror -Wno-unused-parameter
OBJECT_FILES=job_control.o parseline.o promptline.o

shell: src/shell.c builtins.o $(OBJECT_FILES)
	gcc $(COMPILER_FLAGS) src/shell.c builtins.o $(OBJECT_FILES) -o shell

debug: src/shell.c builtins.o $(OBJECT_FILES)
	gcc $(COMPILER_FLAGS) src/shell.c builtins.o $(OBJECT_FILES) -o debug -DDEBUG

glosh: src/shell.c builtins_glorp.o $(OBJECT_FILES)
	gcc $(COMPILER_FLAGS) src/shell.c builtins_glorp.o $(OBJECT_FILES) -o glosh -DGLORP

builtins.o: src/builtins.c
	gcc -c $(COMPILER_FLAGS) src/builtins.c -o builtins.o

builtins_glorp.o: src/builtins.c
	gcc -c $(COMPILER_FLAGS) src/builtins.c -o builtins_glorp.o -DGLORP

job_control.o: src/job_control.c
	gcc -c $(COMPILER_FLAGS) src/job_control.c -o job_control.o

parseline.o: src/parseline.c
	gcc -c $(COMPILER_FLAGS) src/parseline.c -o parseline.o

promptline.o: src/promptline.c
	gcc -c $(COMPILER_FLAGS) src/promptline.c -o promptline.o

clean:
	rm -f builtins.o builtins_glorp.o $(OBJECT_FILES) shell debug glosh
