#!/bin/bash

#this script calls the code coverage testing program gcov (part of the gcc suite)

#fist clean all object files
make -C .. clean

#compile all the c files, link etc
make -C .. tests 

# run the example.out program ... with test coverage (see makefile for flags)
./xelp_unit_tests.out

echo ""
# gcov is the gcc suite test coverage program.  We're interested in the coverage
# the xelp.c file.  
gcov ../src/xelp.c

# now the code coverage is in this file:
# xelp.c.gcov  
# which can be viewed in any text editor 
