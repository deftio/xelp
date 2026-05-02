# Makefile for scripting xelp example (Linux / macOS)
#
# Usage:
#   make            # build and run
#   make build      # build only
#   make clean      # remove build artifacts

CC      = gcc
CFLAGS  = -Wall -Wextra -I../../src
SRC     = scripting-example.c ../../src/xelp.c

BUILD_DIR ?= build
TARGET     = $(BUILD_DIR)/scripting-example

.PHONY: all build run clean

all: build run

build: $(TARGET)

$(TARGET): $(SRC) ../../src/xelp.h ../../src/xelpcfg.h
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(SRC) -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(BUILD_DIR)
