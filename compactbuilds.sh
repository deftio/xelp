echo GCC 64bit intel -Os "**************************"
gcc -c f.c -Os -Wall
du -b xelp.o | awk '{print "obj file size: " $1 " bytes"}'
size xelp.o | awk 'FNR==2{print "exec code:     " $1 " bytes"}' 

#echo GCC strip 64bit "**************************"
#strip xelp.o
#du -b  xelp.o
#size xelp.o | awk 'FNR==2{print "exec code: " $1 " bytes"}' 

echo clang 64bit intel -Os "**************************"
clang -c xelp.c -Os -Wall
du -b xelp.o | awk '{print "obj file size: " $1 " bytes"}'
size xelp.o | awk 'FNR==2{print "exec code:     " $1 " bytes"}' 

#echo clang-3.5 strip 64bit "**************************"
#strip xelp.o
#du -b  xelp.o
#size xelp.o | awk 'FNR==2{print "exec code: " $1 " bytes"}' 

echo GCC 32 bit intel -Os  "**************************"
gcc -c xelp.c -Os -m32 -Wall
du -b xelp.o | awk '{print "obj file size: " $1 " bytes"}'
size xelp.o | awk 'FNR==2{print "exec code:     " $1 " bytes"}' 

#echo GCC strip 32bit  "**************************"
#strip xelp.o
#du -b  xelp.o
#size xelp.o | awk 'FNR==2{print "exec code: " $1 " bytes"}' 
echo ARM64 GCC 4.8 "***************************"
aarch64-linux-gnu-gcc -c xelp.c -Os -s -Wall
du -b xelp.o | awk '{print "obj file size: " $1 " bytes"}'
size xelp.o | awk 'FNR==2{print "exec code:     " $1 " bytes"}' 

echo ARM32 GCC "**************************"
arm-none-eabi-gcc -c xelp.c -Os -s  -Wall
du -b xelp.o | awk '{print "obj file size: " $1 " bytes"}'
size xelp.o | awk 'FNR==2{print "exec code:     " $1 " bytes"}' 

echo ARM32 thumb GCC "**************************"
arm-none-eabi-gcc -c xelp.c -Os -mthumb -s   -Wall
du -b xelp.o | awk '{print "obj file size: " $1 " bytes"}'
size xelp.o | awk 'FNR==2{print "exec code:     " $1 " bytes"}' 

echo MSP430  GCC "**************************"
msp430-gcc -c xelp.c -Os -Wall
du -b xelp.o | awk '{print "obj file size: " $1 " bytes"}'
size xelp.o | awk 'FNR==2{print "exec code:     " $1 " bytes"}' 

echo AVR Arduino using avr-gcc w AVR5 instrset  "**************************"
#arduino uno uses ATmega328P which uses the avr5 instruction set
avr-gcc -c xelp.c -Os -s -mmcu=avr5  -Wall
du -b xelp.o | awk '{print "obj file size: " $1 " bytes"}'
size xelp.o | awk 'FNR==2{print "exec code:     " $1 " bytes"}' 

echo PIC18F  using sdcc "**************************"
sdcc -mpic16 -p18f2620 -c xelp.c  --opt-code-size 
du -b xelp.o | awk '{print "obj file size: " $1 " bytes"}'

echo 8051 using SDCC   "**************************"
sdcc -mmcs51  -c xelp.c -minsize --model-small   --opt-code-size
du -b xelp.o | awk '{print "obj file size: " $1 " bytes"}'

echo 68HC11 family using GCC "*************************"
m68hc11-gcc -c xelp.c  -Os -s
du -b xelp.o | awk '{print "obj file size: " $1 " bytes"}'
size xelp.o | awk 'FNR==2{print "exec code:     " $1 " bytes"}' 

echo 6502 family using cc65 "*************************"
cc65 xelp.c  -o xelp.o -Os
du -b xelp.o | awk '{print "obj file size: " $1 " bytes"}'
#size xelp.o | awk 'FNR==2{print "exec code:     " $1 " bytes"}' 

echo powerpc family using gcc "*************************"
powerpc-linux-gnu-gcc-4.8 -c xelp.c -Os
du -b xelp.o | awk '{print "obj file size: " $1 " bytes"}'
size xelp.o | awk 'FNR==2{print "exec code:     " $1 " bytes"}' 

make clean #clean up all the cruft .sym, .asm, etc

#rebuild one last time for current env
gcc -c xelp.c -Os

#show function allocation table
nm xelp.o -n -S