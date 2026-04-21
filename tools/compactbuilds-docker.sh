#!/bin/bash
# compactbuilds-docker.sh -- cross-compile xelp inside Docker container
# Runs inside the Docker image built from Dockerfile.crossbuild.
#
# Builds each target in three configurations:
#   KEY  -- XELP_ENABLE_KEY only (minimal, single-key dispatch)
#   CLI  -- XELP_ENABLE_KEY + CLI + LINE_EDIT + HELP (typical interactive)
#   FULL -- All features (KEY + CLI + LINE_EDIT + HELP + THR)
#
# Output is grouped by word size (8/16/32/64-bit).

set -e

SRC=/xelp/src/xelp.c
INCLUDE="-I/xelp/src"
OBJ=/tmp/xelp.o
CFG_DIR=/tmp/xelp_cfg

SEP="============================================================"

# Accumulate summary rows: "group|target|key_size|cli_size|full_size"
SUMMARY=""

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
# Returns size via stdout; prints nothing else.  Returns "FAIL" on error.

get_text_size() {
    local cc_cmd="$1"
    eval $cc_cmd > /dev/null 2>&1 || true
    if [ ! -f "$OBJ" ]; then
        echo "FAIL"
        return
    fi
    local sz
    sz=$(size "$OBJ" 2>/dev/null | awk 'FNR==2{print $1}')
    echo "${sz:-FAIL}"
    rm -f "$OBJ"
}

# --- Helper: build one target in all three configs -------------------------
# Prints a row and accumulates summary data.

build_target() {
    local group="$1"    # word-size group for summary table
    local label="$2"    # human-readable target name
    local cc_base="$3"  # compiler command without config flags

    local key_sz cli_sz full_sz

    key_sz=$(get_text_size  "$cc_base -DXELP_CONFIG_OVERRIDE -I$CFG_DIR/key")
    cli_sz=$(get_text_size  "$cc_base -DXELP_CONFIG_OVERRIDE -I$CFG_DIR/cli")
    full_sz=$(get_text_size "$cc_base -DXELP_CONFIG_OVERRIDE -I$CFG_DIR/full")

    printf "  %-26s %8s  %8s  %8s\n" "$label" "$key_sz" "$cli_sz" "$full_sz"
    SUMMARY="${SUMMARY}${group}|${label}|${key_sz}|${cli_sz}|${full_sz}\n"
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
    printf "  %-26s %8s  %8s  %8s\n" "Target" "KEY" "CLI" "FULL"
    printf "  %-26s %8s  %8s  %8s\n" "--------------------------" "--------" "--------" "--------"
}

# --- 64-bit targets --------------------------------------------------------

print_header "64-bit targets"

build_target "64-bit" "GCC x86-64" \
    "gcc -c $SRC $INCLUDE -Os -Wall -o $OBJ"

build_target "64-bit" "Clang x86-64" \
    "clang -c $SRC $INCLUDE -Os -Wall -o $OBJ"

build_target "64-bit" "GCC AArch64 (ARM64)" \
    "aarch64-linux-gnu-gcc -c $SRC $INCLUDE -Os -Wall -o $OBJ"

build_target "64-bit" "GCC RISC-V (rv64)" \
    "riscv64-linux-gnu-gcc -c $SRC $INCLUDE -Os -Wall -o $OBJ"

# --- 32-bit targets --------------------------------------------------------

print_header "32-bit targets"

build_target "32-bit" "GCC x86-32" \
    "gcc -c $SRC $INCLUDE -Os -m32 -Wall -o $OBJ"

build_target "32-bit" "TCC x86" \
    "tcc -c $SRC $INCLUDE -o $OBJ"

build_target "32-bit" "GCC ARM32" \
    "arm-none-eabi-gcc -c $SRC $INCLUDE -Os -Wall -o $OBJ"

build_target "32-bit" "GCC ARM32 Thumb" \
    "arm-none-eabi-gcc -c $SRC $INCLUDE -Os -mthumb -Wall -o $OBJ"

build_target "32-bit" "GCC m68k" \
    "m68k-linux-gnu-gcc -c $SRC $INCLUDE -Os -Wall -o $OBJ"

build_target "32-bit" "GCC PowerPC" \
    "powerpc-linux-gnu-gcc -c $SRC $INCLUDE -Os -Wall -o $OBJ"

build_target "32-bit" "GCC RISC-V (rv32)" \
    "riscv64-unknown-elf-gcc -c $SRC $INCLUDE -Os -march=rv32imac -mabi=ilp32 -Wall -o $OBJ"

build_target "32-bit" "GCC Xtensa LX106 (ESP8266)" \
    "xtensa-lx106-elf-gcc -c $SRC $INCLUDE -Os -Wall -o $OBJ"

# --- 16-bit targets --------------------------------------------------------

print_header "16-bit targets"

build_target "16-bit" "GCC MSP430" \
    "msp430-gcc -c $SRC $INCLUDE -Os -Wall -o $OBJ"

build_target "16-bit" "GCC 68HC11" \
    "m68hc11-gcc -c $SRC $INCLUDE -Os -o $OBJ"

build_target "16-bit" "SDCC PIC18F2620" \
    "sdcc -mpic16 -p18f2620 --opt-code-size -c $SRC $INCLUDE -o $OBJ"

# --- 8-bit targets ---------------------------------------------------------

print_header "8-bit targets"

build_target "8-bit" "GCC AVR5 (ATmega328P)" \
    "avr-gcc -c $SRC $INCLUDE -Os -mmcu=avr5 -Wall -o $OBJ"

build_target "8-bit" "GCC AVR ATtiny85" \
    "avr-gcc -c $SRC $INCLUDE -Os -mmcu=attiny85 -Wall -o $OBJ"

build_target "8-bit" "SDCC MCS-51 (8051)" \
    "sdcc -mmcs51 --model-small --opt-code-size -c $SRC $INCLUDE -o $OBJ"

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

for grp in "64-bit" "32-bit" "16-bit" "8-bit"; do
    echo ""
    printf "  %-26s %8s  %8s  %8s\n" "$grp" "KEY" "CLI" "FULL"
    printf "  %-26s %8s  %8s  %8s\n" "--------------------------" "--------" "--------" "--------"
    echo -e "$SUMMARY" | while IFS='|' read -r g label ks cs fs; do
        [ -z "$g" ] && continue
        [ "$g" != "$grp" ] && continue
        printf "  %-26s %8s  %8s  %8s\n" "$label" "$ks" "$cs" "$fs"
    done
done

echo ""
echo "Done."
