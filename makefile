# make file for xelp posix tests (xelp interpreter for embedded systems)
# @author M A Chatterjee <deftio [at] deftio [dot] com>

CC=gcc   	# C compiler to use
CPP=g++		# C++ compiler to use

C_FLAGS=-I. -Wall -Wextra -Werror -g -O0 -fprofile-arcs -ftest-coverage -DXELP_ENABLE_SCRIPT
CPP_FLAGS=-std=c++11 -Wall

LIB_DIR=src
BUILD_DIR=build

INCLUDES=\
    -I$(LIB_DIR)\

.PHONY: help tests clean clean-all clean-fuzz coverage version fuzz fuzz-parsekey fuzz-parse fuzz-buf2argv fuzz-script examples example validate prerelease funcsizes sizes lint build-ref

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
	@echo "  make build-ref    Generate build/build-reference.md with measured sizes"
	@echo "  make sizes        Feature profile compiled sizes (ARM + host)"
	@echo "  make version      Extract and print library version"
	@echo "  make fuzz         Run fuzz tests (requires clang + libFuzzer)"
	@echo "  make lint         Run cppcheck static analysis on src + examples"
	@echo "  make clean        Remove test build artifacts and fuzz corpus growth"
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
	@gcov -b -o $(BUILD_DIR) $(LIB_DIR)/xelp.c
	@mv -f *.gcov $(BUILD_DIR)/ 2>/dev/null || true

coverage: tests
	@echo "--- Coverage Summary ---"
	@gcov -b -o $(BUILD_DIR) $(LIB_DIR)/xelp.c 2>/dev/null | grep -E "File|Lines|Branches|Taken"
	@mv -f *.gcov $(BUILD_DIR)/ 2>/dev/null || true
	@echo "See $(BUILD_DIR)/xelp.c.gcov for line-by-line details (gcov -b)"

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
	@echo "--- Building xelp-script ---"
	$(MAKE) -C examples/xelp-script BUILD_DIR=../../build/examples/xelp-script build
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
			src/ examples/posix-simple/ examples/scripting/ examples/posix-argv/ examples/xelp-script/; \
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
	@echo "--- Coverage gate ---"
	@LINE_PCT=$$(gcov -b -o $(BUILD_DIR) $(LIB_DIR)/xelp.c 2>/dev/null \
		| grep "Lines executed" | head -1 \
		| grep -o '[0-9]*\.[0-9]*'); \
	BRANCH_PCT=$$(gcov -b -o $(BUILD_DIR) $(LIB_DIR)/xelp.c 2>/dev/null \
		| grep "Taken at least once" | head -1 \
		| grep -o '[0-9]*\.[0-9]*'); \
	mv -f *.gcov $(BUILD_DIR)/ 2>/dev/null || true; \
	echo "  Lines:    $${LINE_PCT}%"; \
	echo "  Branches: $${BRANCH_PCT}% taken at least once"; \
	LINE_OK=$$(echo "$${LINE_PCT} >= 100.0" | bc 2>/dev/null || echo 1); \
	BRANCH_OK=$$(echo "$${BRANCH_PCT} >= 97.0" | bc 2>/dev/null || echo 1); \
	if [ "$$LINE_OK" != "1" ]; then echo "FAIL: line coverage < 100%"; exit 1; fi; \
	if [ "$$BRANCH_OK" != "1" ]; then echo "FAIL: branch coverage < 97%"; exit 1; fi
	@echo ""
	@echo "=== Validation passed: lint + tests + coverage + examples build clean ==="

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
	@echo "--- Generating docs/build-reference.md ---"
	bash tools/gen_build_reference.sh
	@echo ""
	@echo "=== Pre-release complete: tests passed, sizes updated ==="

#=======================================================================
# Fuzz testing (requires clang with libFuzzer). Default sanitizer is fuzzer-only so
# `make fuzz` succeeds on hosts where fuzzer+ASan/UBSan misbehaves (e.g. some Xcode setups).
# For stronger fuzzing on Linux/Homebrew LLVM, try:
#   make fuzz FUZZ_SAN=fuzzer,address,undefined FUZZ_CC=/path/to/llvm/clang
FUZZ_CC ?= /usr/local/opt/llvm/bin/clang
FUZZ_SAN ?= fuzzer
FUZZ_FLAGS = -fsanitize=$(FUZZ_SAN) -g -O1 -I$(LIB_DIR)
FUZZ_DIR = tests/fuzz
FUZZ_TIME ?= 60
FUZZ_CORPUS_PARSE_SEEDS = $(FUZZ_DIR)/corpus_parse/seeds
FUZZ_CORPUS_PARSE_GEN = $(FUZZ_DIR)/corpus_parse/generated
FUZZ_CORPUS_PARSEKEY_SEEDS = $(FUZZ_DIR)/corpus_parsekey/seeds
FUZZ_CORPUS_PARSEKEY_GEN = $(FUZZ_DIR)/corpus_parsekey/generated
FUZZ_CORPUS_BUF2ARGV_SEEDS = $(FUZZ_DIR)/corpus_buf2argv/seeds
FUZZ_CORPUS_BUF2ARGV_GEN = $(FUZZ_DIR)/corpus_buf2argv/generated

fuzz-parsekey: | $(BUILD_DIR)
	$(FUZZ_CC) $(FUZZ_FLAGS) $(LIB_DIR)/xelp.c $(FUZZ_DIR)/fuzz_parsekey.c \
		-o $(BUILD_DIR)/fuzz_parsekey
	@mkdir -p $(FUZZ_CORPUS_PARSEKEY_SEEDS) $(FUZZ_CORPUS_PARSEKEY_GEN)
	@cp -n $(FUZZ_CORPUS_PARSEKEY_SEEDS)/* $(FUZZ_CORPUS_PARSEKEY_GEN)/ 2>/dev/null || true
	$(BUILD_DIR)/fuzz_parsekey $(FUZZ_CORPUS_PARSEKEY_GEN) -max_total_time=$(FUZZ_TIME)

fuzz-parse: | $(BUILD_DIR)
	$(FUZZ_CC) $(FUZZ_FLAGS) $(LIB_DIR)/xelp.c $(FUZZ_DIR)/fuzz_parse.c \
		-o $(BUILD_DIR)/fuzz_parse
	@mkdir -p $(FUZZ_CORPUS_PARSE_SEEDS) $(FUZZ_CORPUS_PARSE_GEN)
	@cp -n $(FUZZ_CORPUS_PARSE_SEEDS)/* $(FUZZ_CORPUS_PARSE_GEN)/ 2>/dev/null || true
	$(BUILD_DIR)/fuzz_parse $(FUZZ_CORPUS_PARSE_GEN) -max_total_time=$(FUZZ_TIME)

fuzz-buf2argv: | $(BUILD_DIR)
	$(FUZZ_CC) $(FUZZ_FLAGS) $(LIB_DIR)/xelp.c $(FUZZ_DIR)/fuzz_buf2argv.c \
		-o $(BUILD_DIR)/fuzz_buf2argv
	@mkdir -p $(FUZZ_CORPUS_BUF2ARGV_SEEDS) $(FUZZ_CORPUS_BUF2ARGV_GEN)
	@cp -n $(FUZZ_CORPUS_BUF2ARGV_SEEDS)/* $(FUZZ_CORPUS_BUF2ARGV_GEN)/ 2>/dev/null || true
	$(BUILD_DIR)/fuzz_buf2argv $(FUZZ_CORPUS_BUF2ARGV_GEN) -max_total_time=$(FUZZ_TIME)

FUZZ_CORPUS_SCRIPT_SEEDS = $(FUZZ_DIR)/corpus_script/seeds
FUZZ_CORPUS_SCRIPT_GEN = $(FUZZ_DIR)/corpus_script/generated

fuzz-script: | $(BUILD_DIR)
	$(FUZZ_CC) $(FUZZ_FLAGS) -DXELP_ENABLE_SCRIPT $(LIB_DIR)/xelp.c $(FUZZ_DIR)/fuzz_script.c \
		-o $(BUILD_DIR)/fuzz_script
	@mkdir -p $(FUZZ_CORPUS_SCRIPT_SEEDS) $(FUZZ_CORPUS_SCRIPT_GEN)
	@cp -n $(FUZZ_CORPUS_SCRIPT_SEEDS)/* $(FUZZ_CORPUS_SCRIPT_GEN)/ 2>/dev/null || true
	$(BUILD_DIR)/fuzz_script $(FUZZ_CORPUS_SCRIPT_GEN) -max_total_time=$(FUZZ_TIME)

fuzz: fuzz-parsekey fuzz-parse fuzz-buf2argv fuzz-script

#=======================================================================
# Build reference document with measured sizes and sizeof(XELP)
build-ref:
	@bash tools/gen_build_reference.sh

#=======================================================================
# Per-function compiled sizes (x86-32 and ARM32 if available)
funcsizes:
	@bash tools/funcsizes.sh

#=======================================================================
# Feature profile compiled sizes (ARM Cortex-M0 via Docker + host GCC)
sizes:
	@bash dev/size_profiles.sh

#=======================================================================
# clean-fuzz -- remove libFuzzer-grown corpus files (keep seeds/)
clean-fuzz:
	-rm -rf $(FUZZ_CORPUS_PARSE_GEN) $(FUZZ_CORPUS_PARSEKEY_GEN) $(FUZZ_CORPUS_BUF2ARGV_GEN) $(FUZZ_CORPUS_SCRIPT_GEN)
	-find $(FUZZ_DIR)/corpus_parse $(FUZZ_DIR)/corpus_parsekey $(FUZZ_DIR)/corpus_buf2argv $(FUZZ_DIR)/corpus_script \
		-maxdepth 1 -type f -exec rm -f {} + 2>/dev/null || true
	@find $(FUZZ_DIR)/corpus_parse/seeds $(FUZZ_DIR)/corpus_parsekey/seeds \
		$(FUZZ_DIR)/corpus_buf2argv/seeds $(FUZZ_DIR)/corpus_script/seeds -maxdepth 1 -type f 2>/dev/null | \
		grep -E '/[0-9a-f]{40}$$' | xargs rm -f

#=======================================================================
# clean -- wipe all build artifacts (src/ stays clean)
clean: clean-fuzz
	-rm -rf $(BUILD_DIR)
	-rm -f *.gcov *.gcda *.gcno

# clean-all -- clean tests + all examples
clean-all: clean
	-$(MAKE) -C examples/posix-simple BUILD_DIR=../../build/examples/posix-simple clean
	-$(MAKE) -C examples/posix-simple-cpp BUILD_DIR=../../build/examples/posix-simple-cpp clean
	-$(MAKE) -C examples/scripting BUILD_DIR=../../build/examples/scripting clean
	-$(MAKE) -C examples/posix-argv BUILD_DIR=../../build/examples/posix-argv clean
	-$(MAKE) -C examples/xelp-script BUILD_DIR=../../build/examples/xelp-script clean

