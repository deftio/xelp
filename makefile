# make file for xelp posix tests (simple interpreter for embedded systems)
# @author M A Chatterjee <deftio [at] deftio [dot] com>

CC=gcc
CPP=g++

C_FLAGS=-I. -Wall 
TEST_FLAGS=-fprofile-arcs -ftest-coverage # coverage and profiling data
CPP_FLAGS=-std=c++11 -Wall

LIB_DIR=src
TEST_DIR=tests
EXAMPLE_POSIX_DIR=examples/posix-simple

INCLUDES=\
    -I$(LIB_DIR)\

.PHONY: tests clean example

%.o: %.c
	$(CC) $(C_FLAGS)  $(INCLUDES) -c $< -o $@

#=======================================================================
#build unit tests in /tests folder 
SRC_TESTS=\
    $(LIB_DIR)/xelp.c\
	$(TEST_DIR)/xelp_unit_tests.c

OBJC_TESTS=$(SRC_TESTS:.c=.o)  # object files for C language (not CPP) for tests

tests: $(OBJC_TESTS)
	$(CC) $(C_FLAGS) $(INCLUDES) $(TEST_FLAGS) $(OBJC_TESTS) -o $(TEST_DIR)/xelp_unit_tests.out
	@$(TEST_DIR)/xelp_unit_tests.out

#=======================================================================
#build simple example in /example/posix-simple folder
SRC_EXAMPLE1=\
	$(LIB_DIR)/xelp.c\
	$(EXAMPLE_POSIX_DIR)/xelp-example.c	

OBJC_EXAMPLE1=$(SRC_EXAMPLE1:.c=.o)  # object files for C language (not CPP) for tests

example: $(OBJC_EXAMPLE1)
	$(CC) $(C_FLAGS) $(INCLUDES) $(OBJC_EXAMPLE1) -o $(EXAMPLE_POSIX_DIR)/xelp-example.out -lm -lncurses -Os 
	@$(EXAMPLE_POSIX_DIR)/xelp-example.out

#=======================================================================
clean:
	rm $(OBJ_TESTS) $(OBJC_EXAMPLE1) *.o *.out *.asm  *.lst *.sym *.rel *.s *.gcov *.gcno *.gcda -f

