# ============================================================================
#  RAMPYAARYAN COMPILER - Makefile
#  Build the native Rampyaaryan binary
# ============================================================================

# Compiler
CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=gnu99
LDFLAGS = -lm

# Directories
SRC_DIR = src
BUILD_DIR = build
BIN_DIR = bin

# Target
TARGET = $(BIN_DIR)/rampyaaryan

# Windows adjustments
ifeq ($(OS),Windows_NT)
    TARGET = $(BIN_DIR)/rampyaaryan.exe
    RM = del /Q /F
    RMDIR = rmdir /S /Q
    MKDIR = if not exist "$(subst /,\,$1)" mkdir "$(subst /,\,$1)"
    SEP = \\
else
    RM = rm -f
    RMDIR = rm -rf
    MKDIR = mkdir -p $1
    SEP = /
endif

# Source files
SOURCES = \
    $(SRC_DIR)/memory.c \
    $(SRC_DIR)/value.c \
    $(SRC_DIR)/object.c \
    $(SRC_DIR)/table.c \
    $(SRC_DIR)/chunk.c \
    $(SRC_DIR)/lexer.c \
    $(SRC_DIR)/compiler.c \
    $(SRC_DIR)/vm.c \
    $(SRC_DIR)/native.c \
    $(SRC_DIR)/debug.c \
    $(SRC_DIR)/ascii_art.c \
    $(SRC_DIR)/main.c

# Object files
OBJECTS = $(SOURCES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

# ============================================================================
#  TARGETS
# ============================================================================

.PHONY: all clean install uninstall debug release

all: $(TARGET)
	@echo ""
	@echo "  Build complete! Binary: $(TARGET)"
	@echo "  Run: $(TARGET) --help"
	@echo ""

$(TARGET): $(OBJECTS)
	$(call MKDIR,$(BIN_DIR))
	$(CC) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(call MKDIR,$(BUILD_DIR))
	$(CC) $(CFLAGS) -c $< -o $@

# Debug build (with trace output)
debug: CFLAGS += -g -DDEBUG_TRACE_EXECUTION -DDEBUG_STRESS_GC
debug: $(TARGET)

# Release build (optimized)
release: CFLAGS = -Wall -O3 -std=gnu99 -DNDEBUG
release: $(TARGET)

clean:
ifeq ($(OS),Windows_NT)
	@if exist "$(subst /,\,$(BUILD_DIR))" $(RMDIR) "$(subst /,\,$(BUILD_DIR))"
	@if exist "$(subst /,\,$(BIN_DIR))" $(RMDIR) "$(subst /,\,$(BIN_DIR))"
else
	$(RMDIR) $(BUILD_DIR) $(BIN_DIR)
endif

# Install (Unix-like systems)
install: $(TARGET)
ifeq ($(OS),Windows_NT)
	@echo "On Windows, add $(BIN_DIR) to your PATH manually."
else
	install -d /usr/local/bin
	install -m 755 $(TARGET) /usr/local/bin/rampyaaryan
	@echo "Installed to /usr/local/bin/rampyaaryan"
endif

uninstall:
ifeq ($(OS),Windows_NT)
	@echo "Remove rampyaaryan.exe from your PATH manually."
else
	$(RM) /usr/local/bin/rampyaaryan
	@echo "Uninstalled rampyaaryan"
endif

# ============================================================================
#  DEPENDENCIES (auto-generated would be ideal, but manual is fine for now)
# ============================================================================
$(BUILD_DIR)/memory.o:    $(SRC_DIR)/memory.c $(SRC_DIR)/memory.h $(SRC_DIR)/common.h $(SRC_DIR)/vm.h
$(BUILD_DIR)/value.o:     $(SRC_DIR)/value.c $(SRC_DIR)/value.h $(SRC_DIR)/common.h $(SRC_DIR)/object.h
$(BUILD_DIR)/object.o:    $(SRC_DIR)/object.c $(SRC_DIR)/object.h $(SRC_DIR)/common.h $(SRC_DIR)/memory.h
$(BUILD_DIR)/table.o:     $(SRC_DIR)/table.c $(SRC_DIR)/table.h $(SRC_DIR)/common.h $(SRC_DIR)/memory.h
$(BUILD_DIR)/chunk.o:     $(SRC_DIR)/chunk.c $(SRC_DIR)/chunk.h $(SRC_DIR)/common.h $(SRC_DIR)/memory.h
$(BUILD_DIR)/lexer.o:     $(SRC_DIR)/lexer.c $(SRC_DIR)/lexer.h $(SRC_DIR)/common.h
$(BUILD_DIR)/compiler.o:  $(SRC_DIR)/compiler.c $(SRC_DIR)/compiler.h $(SRC_DIR)/common.h
$(BUILD_DIR)/vm.o:        $(SRC_DIR)/vm.c $(SRC_DIR)/vm.h $(SRC_DIR)/common.h $(SRC_DIR)/compiler.h
$(BUILD_DIR)/native.o:    $(SRC_DIR)/native.c $(SRC_DIR)/native.h $(SRC_DIR)/vm.h
$(BUILD_DIR)/debug.o:     $(SRC_DIR)/debug.c $(SRC_DIR)/debug.h $(SRC_DIR)/chunk.h
$(BUILD_DIR)/ascii_art.o: $(SRC_DIR)/ascii_art.c $(SRC_DIR)/ascii_art.h $(SRC_DIR)/common.h
$(BUILD_DIR)/main.o:      $(SRC_DIR)/main.c $(SRC_DIR)/vm.h $(SRC_DIR)/ascii_art.h
