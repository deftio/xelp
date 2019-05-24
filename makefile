# make file for xelp posix tests (xelp interpreter for embedded systems)
# @author M A Chatterjee <deftio [at] deftio [dot] com>

CC=gcc   	# C compiler to use
CPP=g++		# C++ compiler to use

#C_FLAGS=-I. -Wall -g
C_FLAGS=-I. -Wall -g -fprofile-arcs -ftest-coverage 
CPP_FLAGS=-std=c++11 -Wall

LIB_DIR=src

INCLUDES=\
    -I$(LIB_DIR)\

.PHONY: tests clean example

%.o: %.c
	$(CC) $(C_FLAGS)  $(INCLUDES) -c $< -o $@

#=======================================================================
#build unit tests in /tests folder 
TEST_DIR=tests
TEST_FLAGS=-fprofile-arcs -ftest-coverage -g # coverage and profiling data
SRC_TESTS=\
    $(LIB_DIR)/xelp.c\
	$(TEST_DIR)/xelp_simple_unit_test_fw.c\
	$(TEST_DIR)/xelp_unit_tests.c

OBJC_TESTS=$(SRC_TESTS:.c=.o)  # object files for C language (not CPP) for tests

tests: $(OBJC_TESTS)
	$(CC) $(C_FLAGS) $(INCLUDES) $(TEST_FLAGS) $(OBJC_TESTS) -o $(TEST_DIR)/xelp_unit_tests.out
	@$(TEST_DIR)/xelp_unit_tests.out
	@$(.)gcov src/xelp.c $(LIB_DIR)/xelp.c
	@$(.)mv xelp.c.gcov $(TEST_DIR)
	# @$(TEST_DIR)lcov xelp.c.gcov   ///lcov --coverage --directory . --output-file main_coverage.info
	# genhtml xelp.info --output-directory $(TEST_DIR)

#=======================================================================
#build simple example in /example/posix-simple folder
EXAMPLE_POSIX_DIR=examples/posix-simple
SRC_EXAMPLE1=\
	$(LIB_DIR)/xelp.c\
	$(EXAMPLE_POSIX_DIR)/xelp-example.c	

OBJC_EXAMPLE1=$(SRC_EXAMPLE1:.c=.o)  # object files for C language (not CPP) for tests

example: $(OBJC_EXAMPLE1)
	$(CC) $(C_FLAGS) $(INCLUDES) $(OBJC_EXAMPLE1) -o $(EXAMPLE_POSIX_DIR)/xelp-example.out -lm -lncurses -Os 
	@$(EXAMPLE_POSIX_DIR)/xelp-example.out

#=======================================================================
# clean deletes all compiled object files and debugging / profiling / listing files
clean:
	rm $(OBJC_TESTS) $(OBJC_EXAMPLE1) -f
	rm $(TEST_DIR)/*.gcda  $(TEST_DIR)/*.gcno $(TEST_DIR)/*.gcov $(TEST_DIR)/*.out -f 
	rm $(EXAMPLE_POSIX_DIR)/*.gcda  $(EXAMPLE_POSIX_DIR)/*.gcno $(EXAMPLE_POSIX_DIR)/*.gcov $(EXAMPLE_POSIX_DIR)/*.out -f 
	rm $(LIB_DIR)/*.gcda  $(LIB_DIR)/*.gcno $(LIB_DIR)/*.gcov  $(LIB_DIR)/*.out $(LIB_DIR)/*.asm $(LIB_DIR)/*.rel $(LIB_DIR)/*.s $(LIB_DIR)/*.asm -f 


