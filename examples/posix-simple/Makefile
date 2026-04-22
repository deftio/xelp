# Makefile for posix-simple xelp example (Linux / macOS)
#
# Usage:
#   make            # build and run
#   make build      # build only
#   make clean      # remove build artifacts

CC      = gcc
CFLAGS  = -Wall -Wextra -I../../src -Os
LDFLAGS = -lncurses -lm
SRC     = xelp-example.c ../../src/xelp.c
TARGET  = xelp-example

.PHONY: all build run clean

all: build run

build: $(TARGET)

$(TARGET): $(SRC) ../../src/xelp.h ../../src/xelpcfg.h
	$(CC) $(CFLAGS) $(SRC) $(LDFLAGS) -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
