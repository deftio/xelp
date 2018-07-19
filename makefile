# make file for xelp posix tests (simple interpreter for embedded systems)
# @author M A Chatterjee <deftio [at] deftio [dot] com>

CC=gcc
CFLAGS=-I. -Wall
DEPS= xelp.h
OBJ= xelp.o xelp-example.o  
INC=$(.) $(./tests)
                  
%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

#posix demo test program
xelp-example.out: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) -lm -lncurses -Os

#clean up object files, useful before check-ins etc
clean :
	rm  *.o *.out *.asm  *.lst *.sym *.rel *.s -f

#make unit test (posix version)
xelp-unit-tests.o: ./tests/xelp_unit_tests.c 
	$(CC) -c -o $@ $< $(CFLAGS)

OBJTEST= xelp.o xelp-unit-tests.o 

xelp-unit-tests.out: $(OBJTEST)
	gcc -o $@ $^ $(INC) $(CFLAGS) -lm  -Os 


