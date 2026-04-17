#!/bin/bash
# compactbuilds-docker.sh -- cross-compile xelp inside Docker container
# Runs inside the Docker image built from Dockerfile.crossbuild.
# Reports object file and .text section sizes for each target.

set -e

SRC=/xelp/src/xelp.c
INCLUDE="-I/xelp/src"
OBJ=/tmp/xelp.o

# Separator width
SEP="============================================================"

print_sizes() {
    local label="$1"
    echo ""
    echo "$SEP"
    echo "$label"
    echo "$SEP"
    if [ ! -f "$OBJ" ]; then
        echo "  (build failed)"
        return
    fi
    OBJ_SIZE=$(stat -c%s "$OBJ" 2>/dev/null || wc -c < "$OBJ")
    TEXT_SIZE=$(size "$OBJ" 2>/dev/null | awk 'FNR==2{print $1}')
    printf "  obj file size: %6s bytes\n" "$OBJ_SIZE"
    printf "  .text section: %6s bytes\n" "$TEXT_SIZE"
    rm -f "$OBJ"
}

echo ""
echo "xelp cross-compilation size report"
echo "Date: $(date -u '+%Y-%m-%d %H:%M UTC')"
echo ""

# --- x86-64 ---
gcc -c $SRC $INCLUDE -Os -Wall -o $OBJ 2>&1 && true
print_sizes "GCC x86-64 -Os"

clang -c $SRC $INCLUDE -Os -Wall -o $OBJ 2>&1 && true
print_sizes "Clang x86-64 -Os"

# --- x86-32 ---
gcc -c $SRC $INCLUDE -Os -m32 -Wall -o $OBJ 2>&1 && true
print_sizes "GCC x86-32 -Os"

tcc -c $SRC $INCLUDE -o $OBJ 2>&1 && true
print_sizes "TCC (Tiny C Compiler)"

# --- ARM ---
aarch64-linux-gnu-gcc -c $SRC $INCLUDE -Os -Wall -o $OBJ 2>&1 && true
print_sizes "GCC AArch64 (ARM64) -Os"

arm-none-eabi-gcc -c $SRC $INCLUDE -Os -Wall -o $OBJ 2>&1 && true
print_sizes "GCC ARM32 (bare metal) -Os"

arm-none-eabi-gcc -c $SRC $INCLUDE -Os -mthumb -Wall -o $OBJ 2>&1 && true
print_sizes "GCC ARM32 Thumb -Os"

# --- MSP430 ---
msp430-gcc -c $SRC $INCLUDE -Os -Wall -o $OBJ 2>&1 && true
print_sizes "GCC MSP430 -Os"

# --- AVR ---
avr-gcc -c $SRC $INCLUDE -Os -mmcu=avr5 -Wall -o $OBJ 2>&1 && true
print_sizes "GCC AVR5 (ATmega328P) -Os"

avr-gcc -c $SRC $INCLUDE -Os -mmcu=attiny85 -Wall -o $OBJ 2>&1 && true
print_sizes "GCC AVR ATtiny85 -Os"

# --- 68HC11 ---
m68hc11-gcc -c $SRC $INCLUDE -Os -o $OBJ 2>&1 && true
print_sizes "GCC 68HC11 -Os"

# --- PowerPC ---
powerpc-linux-gnu-gcc -c $SRC $INCLUDE -Os -Wall -o $OBJ 2>&1 && true
print_sizes "GCC PowerPC -Os"

# --- Summary function table (native GCC) ---
echo ""
echo "$SEP"
echo "Function size table (GCC x86-64)"
echo "$SEP"
gcc -c $SRC $INCLUDE -Os -Wall -o $OBJ 2>&1
nm $OBJ -n -S --size-sort -f sysv -t d 2>/dev/null | grep -E "FUNC" || true
rm -f $OBJ

echo ""
echo "Done."
