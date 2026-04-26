# Makefile for posix-simple-cpp xelp example (Linux / macOS)
#
# Uses the XelpCLI C++ wrapper with Easy API (commands({...}) with lambdas).
# Requires C++17 for inline static members in XelpArduino.h.
#
# Usage:
#   make            # build and run
#   make build      # build only
#   make clean      # remove build artifacts

CC      = gcc
CXX     = g++
CFLAGS  = -Wall -Wextra -I../../src -Os
CXXFLAGS = -std=c++17 -Wall -Wextra -I../../src -Os
LDFLAGS = -lncurses -lm
TARGET  = xelp-example-cpp

.PHONY: all build run clean

all: build run

build: $(TARGET)

xelp.o: ../../src/xelp.c ../../src/xelp.h ../../src/xelpcfg.h
	$(CC) $(CFLAGS) -c ../../src/xelp.c -o $@

xelp-example-cpp.o: xelp-example-cpp.cpp ../../src/xelp.h ../../src/xelpcfg.h ../../src/XelpArduino.h
	$(CXX) $(CXXFLAGS) -c xelp-example-cpp.cpp -o $@

$(TARGET): xelp.o xelp-example-cpp.o
	$(CXX) xelp.o xelp-example-cpp.o $(LDFLAGS) -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) *.o
