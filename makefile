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

.PHONY: tests clean example coverage version

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
#build simple example in /example/posix-simple folder
EXAMPLE_POSIX_DIR=examples/posix-simple

OBJ_EXAMPLE1=\
    $(BUILD_DIR)/xelp_ex.o\
    $(BUILD_DIR)/xelp-example.o

$(BUILD_DIR)/xelp_ex.o: $(LIB_DIR)/xelp.c $(LIB_DIR)/xelp.h $(LIB_DIR)/xelpcfg.h | $(BUILD_DIR)
	$(CC) $(C_FLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/xelp-example.o: $(EXAMPLE_POSIX_DIR)/xelp-example.c | $(BUILD_DIR)
	$(CC) $(C_FLAGS) $(INCLUDES) -c $< -o $@

example: $(OBJ_EXAMPLE1)
	$(CC) $(C_FLAGS) $(INCLUDES) $(OBJ_EXAMPLE1) -o $(BUILD_DIR)/xelp-example.out -lm -lncurses -Os
	@$(BUILD_DIR)/xelp-example.out

#=======================================================================
# clean -- wipe all build artifacts (src/ stays clean)
clean:
	-rm -rf $(BUILD_DIR)
	-rm -f *.gcov *.gcda *.gcno

