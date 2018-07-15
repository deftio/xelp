# make file for xelp posix tests (simple interpreter for embedded systems)
# @author M A Chatterjee <deftio [at] deftio [dot] com>

CC=gcc
CFLAGS=-I. -Wall
DEPS = xelp.h
OBJ = xelp.o xelp-example.o  

                  
%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

xelp-example.out: $(OBJ)
	gcc -o $@ $^ $(CFLAGS) -lm -lncurses -Os

clean :
	rm  *.o *.out *.asm  *.lst *.sym *.rel *.s -f

