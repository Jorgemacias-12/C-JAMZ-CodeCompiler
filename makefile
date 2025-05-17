# Makefile for C-JAMZ-CodeCompiler

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -Iinclude

# Directories and files
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin
MAIN = main.c
OUTPUT = $(BIN_DIR)/cjamz

# Sources and objects
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

# Default target
all: $(OUTPUT)

# Build executable
$(OUTPUT): $(MAIN) $(OBJS) | $(BIN_DIR)
	@echo "Compiling program"
	@$(CC) $(CFLAGS) -o $@ $^

# Compile source files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Ensure directories exist
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Run the compiled binary silently
run:
	@$(MAKE) -s all
	@echo "Running C-JAMZ-CodeCompiler..."
	@./$(OUTPUT)

# Clean up
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

dist-clean: clean
	find . -name "*~" -type f -delete

.PHONY: all clean dist-clean run
