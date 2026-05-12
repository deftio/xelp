# make file for xelp posix tests (xelp interpreter for embedded systems)
# @author M A Chatterjee <deftio [at] deftio [dot] com>

CC=gcc   	# C compiler to use
CPP=g++		# C++ compiler to use

C_FLAGS=-I. -Wall -Wextra -Werror -g -fprofile-arcs -ftest-coverage
CPP_FLAGS=-std=c++11 -Wall

LIB_DIR=src
BUILD_DIR=build

INCLUDES=\
    -I$(LIB_DIR)\

.PHONY: help tests clean clean-all coverage version fuzz fuzz-parsekey fuzz-parse fuzz-buf2argv examples example validate prerelease funcsizes sizes lint

#=======================================================================
# Default target: print available targets
help:
	@echo "xelp build targets:"
	@echo ""
	@echo "  make validate     Lint + build + run tests + build examples (pre-push check)"
	@echo "  make prerelease   Validate + cross-build sizes + update README tables"
	@echo "  make tests        Build + run unit tests with coverage"
	@echo "  make examples     Build all examples (no interactive launch)"
	@echo "  make example      Build + run the posix ncurses demo (interactive)"
	@echo "  make coverage     Tests + coverage summary"
	@echo "  make funcsizes    Per-function compiled sizes (x86-32, ARM32)"
	@echo "  make sizes        Feature profile compiled sizes (ARM + host)"
	@echo "  make version      Extract and print library version"
	@echo "  make fuzz         Run fuzz tests (requires clang + libFuzzer)"
	@echo "  make lint         Run cppcheck static analysis on src + examples"
	@echo "  make clean        Remove test build artifacts"
	@echo "  make clean-all    Remove all build artifacts including examples"
	@echo ""

# all object files go in build/
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

#=======================================================================
#build unit tests in /tests folder
TEST_DIR=tests

OBJ_TESTS=\
    $(BUILD_DIR)/xelp.o\
    $(BUILD_DIR)/jumpbug_unit_test_fw.o\
    $(BUILD_DIR)/xelp_unit_tests.o

$(BUILD_DIR)/xelp.o: $(LIB_DIR)/xelp.c $(LIB_DIR)/xelp.h $(LIB_DIR)/xelpcfg.h | $(BUILD_DIR)
	$(CC) $(C_FLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/jumpbug_unit_test_fw.o: $(TEST_DIR)/jumpbug_unit_test_fw.c | $(BUILD_DIR)
	$(CC) $(C_FLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/xelp_unit_tests.o: $(TEST_DIR)/xelp_unit_tests.c $(LIB_DIR)/xelp.h $(LIB_DIR)/xelpcfg.h | $(BUILD_DIR)
	$(CC) $(C_FLAGS) $(INCLUDES) -c $< -o $@

tests: $(OBJ_TESTS)
	$(CC) $(C_FLAGS) $(INCLUDES) $(OBJ_TESTS) -o $(BUILD_DIR)/xelp_unit_tests.out
	@$(BUILD_DIR)/xelp_unit_tests.out
	@gcov -o $(BUILD_DIR) $(LIB_DIR)/xelp.c
	@mv -f xelp.c.gcov $(BUILD_DIR)/ 2>/dev/null || true

coverage: tests
	@echo "--- Coverage Summary ---"
	@gcov -o $(BUILD_DIR) $(LIB_DIR)/xelp.c 2>/dev/null | grep -A 1 "File.*xelp.c"
	@echo "See $(BUILD_DIR)/xelp.c.gcov for line-by-line details"

version:
	@mkdir -p $(BUILD_DIR)
	@$(CC) tools/extract_version.c -I$(LIB_DIR) -o $(BUILD_DIR)/extract_version
	@$(BUILD_DIR)/extract_version $(BUILD_DIR)/xelp_version.yaml
	@cat $(BUILD_DIR)/xelp_version.yaml

#=======================================================================
# Build examples (build-only, no interactive launch)
# posix-simple and posix-simple-cpp require ncurses

examples:
	@echo "--- Building posix-simple ---"
	$(MAKE) -C examples/posix-simple BUILD_DIR=../../build/examples/posix-simple build
	@echo "--- Building posix-simple-cpp ---"
	$(MAKE) -C examples/posix-simple-cpp BUILD_DIR=../../build/examples/posix-simple-cpp build
	@echo "--- Building scripting ---"
	$(MAKE) -C examples/scripting BUILD_DIR=../../build/examples/scripting build
	@echo "--- Building posix-argv ---"
	$(MAKE) -C examples/posix-argv BUILD_DIR=../../build/examples/posix-argv build
	@echo "--- All examples built ---"

# Build and run the posix ncurses demo (interactive)
example:
	$(MAKE) -C examples/posix-simple BUILD_DIR=../../build/examples/posix-simple

#=======================================================================
# Static analysis with cppcheck

lint:
	@if command -v cppcheck >/dev/null 2>&1; then \
		echo "--- cppcheck: src + examples (C) ---"; \
		cppcheck --enable=warning,performance,portability \
			--error-exitcode=1 \
			--suppress=missingIncludeSystem \
			-I src \
			src/ examples/posix-simple/ examples/scripting/ examples/posix-argv/; \
		echo "--- cppcheck: examples (C++) ---"; \
		cppcheck --enable=warning,performance,portability \
			--error-exitcode=1 \
			--suppress=missingIncludeSystem \
			-I src \
			--language=c++ \
			examples/posix-simple-cpp/xelp-example-cpp.cpp; \
		echo "--- cppcheck passed ---"; \
	else \
		echo "--- cppcheck not found, skipping lint (install: brew/apt install cppcheck) ---"; \
	fi

#=======================================================================
# Local validation: lint + tests + examples build (no Docker, no release)
# Use this for day-to-day development before pushing.

validate: lint tests examples
	@echo ""
	@echo "=== Validation passed: lint + tests + examples build clean ==="

#=======================================================================
# Pre-release: validate + cross-compile sizes + update README tables
# Requires Docker for the cross-build step.

prerelease: validate
	@echo ""
	@echo "--- Cross-compiling all targets (Docker required) ---"
	bash tools/crossbuild.sh
	@echo ""
	@echo "--- Updating size tables in README.md and pages/index.html ---"
	bash tools/update_sizes.sh
	@echo ""
	@echo "=== Pre-release complete: tests passed, sizes updated ==="

#=======================================================================
# Fuzz testing (requires clang with libFuzzer + ASan + UBSan)
# Override FUZZ_CC for your system, e.g.: make fuzz FUZZ_CC=clang
FUZZ_CC ?= /usr/local/opt/llvm/bin/clang
FUZZ_FLAGS = -fsanitize=fuzzer,address,undefined -g -O1 -I$(LIB_DIR)
FUZZ_DIR = tests/fuzz
FUZZ_TIME ?= 60

fuzz-parsekey: | $(BUILD_DIR)
	$(FUZZ_CC) $(FUZZ_FLAGS) $(LIB_DIR)/xelp.c $(FUZZ_DIR)/fuzz_parsekey.c \
		-o $(BUILD_DIR)/fuzz_parsekey
	@mkdir -p $(FUZZ_DIR)/corpus_parsekey
	$(BUILD_DIR)/fuzz_parsekey $(FUZZ_DIR)/corpus_parsekey -max_total_time=$(FUZZ_TIME)

fuzz-parse: | $(BUILD_DIR)
	$(FUZZ_CC) $(FUZZ_FLAGS) $(LIB_DIR)/xelp.c $(FUZZ_DIR)/fuzz_parse.c \
		-o $(BUILD_DIR)/fuzz_parse
	@mkdir -p $(FUZZ_DIR)/corpus_parse
	$(BUILD_DIR)/fuzz_parse $(FUZZ_DIR)/corpus_parse -max_total_time=$(FUZZ_TIME)

fuzz-buf2argv: | $(BUILD_DIR)
	$(FUZZ_CC) $(FUZZ_FLAGS) $(LIB_DIR)/xelp.c $(FUZZ_DIR)/fuzz_buf2argv.c \
		-o $(BUILD_DIR)/fuzz_buf2argv
	@mkdir -p $(FUZZ_DIR)/corpus_buf2argv
	$(BUILD_DIR)/fuzz_buf2argv $(FUZZ_DIR)/corpus_buf2argv -max_total_time=$(FUZZ_TIME)

fuzz: fuzz-parsekey fuzz-parse fuzz-buf2argv

#=======================================================================
# Per-function compiled sizes (x86-32 and ARM32 if available)
funcsizes:
	@bash tools/funcsizes.sh

#=======================================================================
# Feature profile compiled sizes (ARM Cortex-M0 via Docker + host GCC)
sizes:
	@bash dev/size_profiles.sh

#=======================================================================
# clean -- wipe all build artifacts (src/ stays clean)
clean:
	-rm -rf $(BUILD_DIR)
	-rm -f *.gcov *.gcda *.gcno

# clean-all -- clean tests + all examples
clean-all: clean
	-$(MAKE) -C examples/posix-simple BUILD_DIR=../../build/examples/posix-simple clean
	-$(MAKE) -C examples/posix-simple-cpp BUILD_DIR=../../build/examples/posix-simple-cpp clean
	-$(MAKE) -C examples/scripting BUILD_DIR=../../build/examples/scripting clean
	-$(MAKE) -C examples/posix-argv BUILD_DIR=../../build/examples/posix-argv clean

