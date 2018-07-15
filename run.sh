#!/bin/bash
# simple bash shell script xelp posix tests (simple interpreter for embedded systems)
# @author M A Chatterjee <deftio [at] deftio [dot] com>

printf "***************************************************************\n"
printf "RUN + COMPILE SCRIPT HEADER ***********************************\n"

#build it
make

#clean up *.o
make clean

#run it
#./xelp-example > testout.txt
./xelp-example.out
