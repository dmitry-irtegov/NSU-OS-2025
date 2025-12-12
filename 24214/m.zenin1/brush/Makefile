
SRC_DIR = src
INCLUDE_DIR = include
BUILD_DIR = build
OBJECTS_DIR = $(BUILD_DIR)/obj

CC = gcc

ALL_SRC = $(shell find $(SRC_DIR) -name "*.c")

main: $(patsubst $(SRC_DIR)/%.c, $(OBJECTS_DIR)/%.o, $(ALL_SRC))
	mkdir -p $(BUILD_DIR)
	$(CC) -o $(BUILD_DIR)/main $^

$(OBJECTS_DIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(dir $@)
	$(CC) -Wall -Wextra -I $(INCLUDE_DIR) -c -o $@ $^

debug: $(ALL_SRC)
	mkdir -p $(BUILD_DIR)
	$(CC) -g -DDEBUG -I $(INCLUDE_DIR) -o $(BUILD_DIR)/main $^

