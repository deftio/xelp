#!/bin/bash
# compactbuilds-docker.sh -- cross-compile xelp inside Docker container
# Runs inside the Docker image built from Dockerfile.crossbuild.
#
# Builds each target in three configurations:
#   KEY  -- XELP_ENABLE_KEY only (minimal, single-key dispatch)
#   CLI  -- XELP_ENABLE_KEY + CLI + LINE_EDIT + HELP (typical interactive)
#   FULL -- All features (KEY + CLI + LINE_EDIT + HELP + THR)
#
# Output is grouped by word size (8/16/32/64-bit), ascending.

set -e

SRC=/xelp/src/xelp.c
INCLUDE="-I/xelp/src"
OBJ=/tmp/xelp.o
CFG_DIR=/tmp/xelp_cfg
CSV_FILE=/xelp/build/sizes.csv

SEP="============================================================"

# Accumulate summary rows: "group|target|key_size|cli_size|full_size"
SUMMARY=""

# CSV accumulator (written to CSV_FILE at end)
mkdir -p "$(dirname "$CSV_FILE")"
CSV_ROWS=""

# --- Create config override headers ----------------------------------------
# XELP_CONFIG_OVERRIDE causes xelpcfg.h to #include "xelp_ovr.h" instead of
# its defaults.  We create three versions and swap -I paths to select one.

mkdir -p "$CFG_DIR/key" "$CFG_DIR/cli" "$CFG_DIR/full"

cat > "$CFG_DIR/key/xelp_ovr.h" << 'EOF'
/* KEY-only config: minimal single-key dispatch, no CLI, no THR */
#define XELP_ENABLE_KEY       1
#define XELPKEY_CLI            (XELPKEY_CTP)
#define XELPKEY_KEY            (XELPKEY_ESC)
#define XELPKEY_THR            (XELPKEY_CTT)
#define XELP_CLI_ESC           ('`')
#define XELP_QUO_ESC           ('\\')
#define XELP_REGS_SZ           4
#define XELPREG int
EOF

cat > "$CFG_DIR/cli/xelp_ovr.h" << 'EOF'
/* CLI config: interactive command line with line editing and help */
#define XELP_ENABLE_KEY       1
#define XELP_ENABLE_CLI       1
#define XELP_ENABLE_LINE_EDIT 1
#define XELP_ENABLE_HELP      1
#define XELPKEY_CLI            (XELPKEY_CTP)
#define XELPKEY_KEY            (XELPKEY_ESC)
#define XELPKEY_THR            (XELPKEY_CTT)
#define XELP_CLI_ESC           ('`')
#define XELP_QUO_ESC           ('\\')
#define XELP_CLI_PROMPT        "xelp>"
#define XELP_HELP_KEY_STR      "\nKey functions\n"
#define XELP_HELP_CLI_STR      "\nCLI functions\n"
#define XELP_HELP_ABT_STR      (ths->mpAboutMsg)
#define XELP_REGS_SZ           4
#define XELPREG int
EOF

cat > "$CFG_DIR/full/xelp_ovr.h" << 'EOF'
/* FULL config: all features enabled */
#define XELP_ENABLE_KEY       1
#define XELP_ENABLE_CLI       1
#define XELP_ENABLE_LINE_EDIT 1
#define XELP_ENABLE_HELP      1
#define XELP_ENABLE_THR       1
#define XELPKEY_CLI            (XELPKEY_CTP)
#define XELPKEY_KEY            (XELPKEY_ESC)
#define XELPKEY_THR            (XELPKEY_CTT)
#define XELP_CLI_ESC           ('`')
#define XELP_QUO_ESC           ('\\')
#define XELP_CLI_PROMPT        "xelp>"
#define XELP_HELP_KEY_STR      "\nKey functions\n"
#define XELP_HELP_CLI_STR      "\nCLI functions\n"
#define XELP_HELP_ABT_STR      (ths->mpAboutMsg)
#define XELP_REGS_SZ           4
#define XELPREG int
EOF

# --- Helper: compile and return .text size ---------------------------------
# Returns size via stdout; prints nothing else.
# Tries multiple strategies to extract code size from different object formats:
#   1. GNU size  (ELF, a.out, COFF)
#   2. SDCC .rel (ASxxxx relocatable -- _CODE area hex size)
#   3. cc65 od65 (cc65 native object format)
# Returns "unavail" if compilation failed or size could not be determined.

get_text_size() {
    local cc_cmd="$1"

    # Clean stale outputs from compilers that may ignore -o
    rm -f xelp.rel xelp.asm xelp.lst xelp.sym xelp.o 2>/dev/null

    eval $cc_cmd > /dev/null 2>&1 || true

    # Some compilers (SDCC, cc65) may ignore -o; check fallback locations
    if [ ! -f "$OBJ" ]; then
        for f in xelp.rel xelp.o; do
            if [ -f "$f" ]; then
                mv "$f" "$OBJ"
                break
            fi
        done
    fi

    if [ ! -f "$OBJ" ]; then
        echo "unavail"
        return
    fi

    local sz=""

    # Strategy 1: GNU size (ELF, a.out, COFF)
    sz=$(size "$OBJ" 2>/dev/null | awk 'FNR==2{print $1}')

    # Strategy 2: SDCC .rel / ASxxxx relocatable format
    # Area definition line: "A _CODE size XXXX flags ..."  (XXXX is hex)
    if [ -z "$sz" ]; then
        local hex_sz
        hex_sz=$(sed -n 's/^A _CODE size \([0-9A-Fa-f]\{1,\}\).*/\1/p' "$OBJ" 2>/dev/null | head -1)
        if [ -n "$hex_sz" ]; then
            sz=$((16#$hex_sz))
        fi
    fi

    # Strategy 3: cc65 object format via od65 --dump-segments
    if [ -z "$sz" ] && command -v od65 >/dev/null 2>&1; then
        sz=$(od65 --dump-segments "$OBJ" 2>/dev/null \
            | awk '/Segment "CODE"/{f=1} f && /Size:/{print $2+0; exit}')
    fi

    echo "${sz:-unavail}"
    rm -f "$OBJ" xelp.asm xelp.lst xelp.sym xelp.rel 2>/dev/null
}

# --- Helper: build one target in all three configs -------------------------
# Prints a row, accumulates summary data, and appends a CSV row.
#
# Usage: build_target <width> <cpu> <compiler> <cc_base>
#   width    -- CPU word size (8, 16, 32, 64)
#   cpu      -- CPU name for docs (e.g. "ARM Thumb", "x86-64")
#   compiler -- Compiler name for docs (e.g. "arm-none-eabi-gcc", "GCC")
#   cc_base  -- Compiler command without config flags

build_target() {
    local width="$1"
    local cpu="$2"
    local compiler="$3"
    local cc_base="$4"
    local label="${cpu} (${compiler})"
    local group="${width}-bit"

    local key_sz cli_sz full_sz

    key_sz=$(get_text_size  "$cc_base -DXELP_CONFIG_OVERRIDE -I$CFG_DIR/key")
    cli_sz=$(get_text_size  "$cc_base -DXELP_CONFIG_OVERRIDE -I$CFG_DIR/cli")
    full_sz=$(get_text_size "$cc_base -DXELP_CONFIG_OVERRIDE -I$CFG_DIR/full")

    printf "  %-34s %8s  %8s  %8s\n" "$label" "$key_sz" "$cli_sz" "$full_sz"
    SUMMARY="${SUMMARY}${group}|${label}|${key_sz}|${cli_sz}|${full_sz}\n"
    CSV_ROWS="${CSV_ROWS}${cpu},${width},${compiler},${key_sz},${cli_sz},${full_sz}\n"
}

# --- Report header ---------------------------------------------------------

echo ""
echo "xelp cross-compilation size report (multi-config)"
echo "Date: $(date -u '+%Y-%m-%d %H:%M UTC')"
echo ""
echo "Configurations:"
echo "  KEY  = XELP_ENABLE_KEY only (minimal single-key dispatch)"
echo "  CLI  = KEY + CLI + LINE_EDIT + HELP (typical interactive)"
echo "  FULL = CLI + THR (all features)"
echo ""
echo "All sizes are .text section bytes, compiled with -Os."

# --- Column header (reused for each group) ---------------------------------

print_header() {
    echo ""
    echo "$SEP"
    echo "$1"
    echo "$SEP"
    printf "  %-34s %8s  %8s  %8s\n" "Target" "KEY" "CLI" "FULL"
    printf "  %-34s %8s  %8s  %8s\n" "----------------------------------" "--------" "--------" "--------"
}

# --- 8-bit targets ---------------------------------------------------------

print_header "8-bit targets"

build_target 8 "AVR (ATmega328P)" "avr-gcc" \
    "avr-gcc -c $SRC $INCLUDE -Os -mmcu=avr5 -Wall -o $OBJ"

build_target 8 "AVR (ATtiny85)" "avr-gcc" \
    "avr-gcc -c $SRC $INCLUDE -Os -mmcu=attiny85 -Wall -o $OBJ"

build_target 8 "MCS-51 (8051)" "SDCC" \
    "sdcc -mmcs51 --model-small --opt-code-size -c $SRC $INCLUDE -o $OBJ"

build_target 8 "6502" "cc65" \
    "cl65 -t none -O --cpu 6502 -c $SRC $INCLUDE -o $OBJ"

build_target 8 "Z80" "SDCC" \
    "sdcc -mz80 --opt-code-size -c $SRC $INCLUDE -o $OBJ"

build_target 8 "6800 (HC08)" "SDCC" \
    "sdcc -mhc08 --opt-code-size -c $SRC $INCLUDE -o $OBJ"

build_target 8 "PIC16F877A" "SDCC" \
    "sdcc -mpic14 -p16f877a --opt-code-size -c $SRC $INCLUDE -o $OBJ"

build_target 8 "PIC18F2620" "SDCC" \
    "sdcc -mpic16 -p18f2620 --opt-code-size -c $SRC $INCLUDE -o $OBJ"

# --- 16-bit targets --------------------------------------------------------

print_header "16-bit targets"

build_target 16 "MSP430" "msp430-gcc" \
    "msp430-gcc -c $SRC $INCLUDE -Os -Wall -o $OBJ"

build_target 16 "68HC11" "m68hc11-gcc" \
    "m68hc11-gcc -c $SRC $INCLUDE -Os -o $OBJ"

build_target 16 "8086" "ia16-elf-gcc" \
    "ia16-elf-gcc -c $SRC $INCLUDE -Os -Wall -o $OBJ"

# --- 32-bit targets --------------------------------------------------------

print_header "32-bit targets"

build_target 32 "x86-32" "GCC" \
    "gcc -c $SRC $INCLUDE -Os -m32 -Wall -o $OBJ"

build_target 32 "x86-32" "TCC" \
    "tcc -c $SRC $INCLUDE -o $OBJ"

build_target 32 "ARM32" "arm-none-eabi-gcc" \
    "arm-none-eabi-gcc -c $SRC $INCLUDE -Os -Wall -o $OBJ"

build_target 32 "ARM Thumb" "arm-none-eabi-gcc" \
    "arm-none-eabi-gcc -c $SRC $INCLUDE -Os -mthumb -Wall -o $OBJ"

build_target 32 "m68k" "m68k-linux-gnu-gcc" \
    "m68k-linux-gnu-gcc -c $SRC $INCLUDE -Os -Wall -o $OBJ"

build_target 32 "PowerPC" "powerpc-linux-gnu-gcc" \
    "powerpc-linux-gnu-gcc -c $SRC $INCLUDE -Os -Wall -o $OBJ"

build_target 32 "RISC-V (rv32)" "riscv64-unknown-elf-gcc" \
    "riscv64-unknown-elf-gcc -c $SRC $INCLUDE -Os -march=rv32imac -mabi=ilp32 -Wall -o $OBJ"

build_target 32 "Xtensa LX106 (ESP8266)" "xtensa-lx106-elf-gcc" \
    "xtensa-lx106-elf-gcc -c $SRC $INCLUDE -Os -Wall -o $OBJ"

build_target 32 "Xtensa LX7 (ESP32-S3)" "xtensa-esp-elf-gcc" \
    "xtensa-esp-elf-gcc -mcpu=esp32s3 -c $SRC $INCLUDE -Os -Wall -o $OBJ"

build_target 32 "MIPS32" "mipsel-linux-gnu-gcc" \
    "mipsel-linux-gnu-gcc -c $SRC $INCLUDE -Os -Wall -o $OBJ"

# --- 64-bit targets --------------------------------------------------------

print_header "64-bit targets"

build_target 64 "x86-64" "GCC" \
    "gcc -c $SRC $INCLUDE -Os -Wall -o $OBJ"

build_target 64 "x86-64" "Clang" \
    "clang -c $SRC $INCLUDE -Os -Wall -o $OBJ"

build_target 64 "AArch64 (ARM64)" "aarch64-linux-gnu-gcc" \
    "aarch64-linux-gnu-gcc -c $SRC $INCLUDE -Os -Wall -o $OBJ"

build_target 64 "RISC-V (rv64)" "riscv64-linux-gnu-gcc" \
    "riscv64-linux-gnu-gcc -c $SRC $INCLUDE -Os -Wall -o $OBJ"

build_target 64 "MIPS64" "mips64el-linux-gnuabi64-gcc" \
    "mips64el-linux-gnuabi64-gcc -c $SRC $INCLUDE -Os -Wall -o $OBJ"

# --- Function size table (native GCC, FULL config) -------------------------

echo ""
echo "$SEP"
echo "Function size table (GCC x86-64, FULL config)"
echo "$SEP"
gcc -c $SRC $INCLUDE -Os -Wall -o $OBJ 2>&1
nm $OBJ -n -S --size-sort -f sysv -t d 2>/dev/null | grep -E "FUNC" || true
rm -f $OBJ

# --- Summary table ---------------------------------------------------------

echo ""
echo "$SEP"
echo "Summary: .text section size (bytes), compiled with -Os"
echo "$SEP"

for grp in "8-bit" "16-bit" "32-bit" "64-bit"; do
    echo ""
    printf "  %-34s %8s  %8s  %8s\n" "$grp" "KEY" "CLI" "FULL"
    printf "  %-34s %8s  %8s  %8s\n" "----------------------------------" "--------" "--------" "--------"
    echo -e "$SUMMARY" | while IFS='|' read -r g label ks cs fs; do
        [ -z "$g" ] && continue
        [ "$g" != "$grp" ] && continue
        printf "  %-34s %8s  %8s  %8s\n" "$label" "$ks" "$cs" "$fs"
    done
done

# --- Write CSV file --------------------------------------------------------

echo ""
echo "Writing CSV to $CSV_FILE ..."
{
    echo "cpu,width,compiler,key,cli,full"
    echo -e "$CSV_ROWS" | sed '/^$/d'
} > "$CSV_FILE"
echo "CSV written: $(wc -l < "$CSV_FILE") rows (including header)."

echo ""
echo "Done."
