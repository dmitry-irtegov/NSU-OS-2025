BUILD_DIR = build
SRC_DIR = src
TARGET_EXEC = $(BUILD_DIR)/shell.out

CC = gcc
CFLAGS = -Wall -O2 -I$(SRC_DIR) -MMD -MP

SOURCES = \
	shell.c \
	io_utils.c \
	prompt.c \
	parse.c \
	launch.c

OBJECTS = $(SOURCES:%.c=$(BUILD_DIR)/%.o)
DEPS = $(SOURCES:%.c=$(BUILD_DIR)/%.d)

$(TARGET_EXEC): $(OBJECTS)
	$(LINK.c) $(OBJECTS) -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(@D)
	$(COMPILE.c) $< -o $@

$(BUILD_DIR)/%.d: $(BUILD_DIR)/%.o
	@: # Needed for GNU Make

include $(DEPS)
