COMPILER_FLAGS=-Wall -Wextra -Werror -Wno-unused-parameter
FILES=src/shell.c src/job_control.c src/promptline.c src/parseline.c

shell: $(FILES)
	gcc $(COMPILER_FLAGS) $(FILES) -o shell

debug: $(FILES)
	gcc $(COMPILER_FLAGS) $(FILES) -o debug -DDEBUG

glosh: $(FILES)
	gcc $(COMPILER_FLAGS) $(FILES) -o glosh -DGLORP
