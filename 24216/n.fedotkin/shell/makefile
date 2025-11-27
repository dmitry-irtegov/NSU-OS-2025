SRC_DIR = src
INCLUDE_DIR = include
BUILD_DIR = build
OBJECTS_DIR = $(BUILD_DIR)/obj

CC = gcc
CFLAGS = -Wall -Wextra -I$(INCLUDE_DIR)
TARGET = shell

SRC = src/core/main.c \
	src/core/shell.c \
	src/parser/parseline.c \
	src/execution/command.c \
	src/execution/pipeline.c \
	src/parser/task_manager.c \
	src/io/readline.c \
	src/execution/launcher.c \
	src/execution/process.c \
	src/scheduler/task.c \
	src/scheduler/task_list.c \
	src/core/error_handler.c

OBJ = $(SRC:$(SRC_DIR)/%.c=$(OBJECTS_DIR)/%.o)

run: $(BUILD_DIR)/$(TARGET)
	./$(BUILD_DIR)/$(TARGET)

$(BUILD_DIR)/$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

$(OBJECTS_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@