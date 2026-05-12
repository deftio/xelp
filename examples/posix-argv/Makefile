# Makefile for posix-argv xelp example (Linux / macOS)
#
# Usage:
#   make            # build and run
#   make build      # build only
#   make clean      # remove build artifacts

CC      = gcc
CFLAGS  = -Wall -Wextra -I../../src
SRC     = posix-argv-example.c ../../src/xelp.c

BUILD_DIR ?= build
TARGET     = $(BUILD_DIR)/posix-argv-example

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
