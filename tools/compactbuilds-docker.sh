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
INCLUDE="-I /xelp/src"
OBJ=/tmp/xelp.o
CFG_DIR=/tmp/xelp_cfg
CSV_FILE=/xelp/build/sizes.csv
LOG_FILE=/xelp/build/crossbuild.log

SEP="============================================================"

# Accumulate summary rows: "group|target|key_size|cli_size|full_size"
SUMMARY=""

# CSV accumulator (written to CSV_FILE at end)
mkdir -p "$(dirname "$CSV_FILE")"
CSV_ROWS=""

# Diagnostic counters (temp file so subshell increments propagate)
DIAG_COUNT_FILE=$(mktemp)
echo "0 0" > "$DIAG_COUNT_FILE"

# Initialize diagnostic log
echo "xelp crossbuild diagnostic log" > "$LOG_FILE"
echo "Date: $(date -u '+%Y-%m-%d %H:%M UTC')" >> "$LOG_FILE"
echo "$SEP" >> "$LOG_FILE"

# --- Create config override headers ----------------------------------------
# XELP_CONFIG_OVERRIDE causes xelpcfg.h to #include "xelp_ovr.h" instead of
# its defaults.  We create three versions and swap -I paths to select one.

mkdir -p "$CFG_DIR/key" "$CFG_DIR/cli" "$CFG_DIR/full"

cat > "$CFG_DIR/key/xelp_ovr.h" << 'EOF'
/* KEY-only config: minimal single-key dispatch, no CLI, no THR */
#undef XELP_ENABLE_CLI
#undef XELP_ENABLE_LINE_EDIT
#undef XELP_ENABLE_HISTORY
#undef XELP_ENABLE_ARGV
#undef XELP_ENABLE_KEY
#undef XELP_ENABLE_THR
#undef XELP_ENABLE_HELP

#define XELP_ENABLE_KEY       1
EOF

cat > "$CFG_DIR/cli/xelp_ovr.h" << 'EOF'
/* CLI config: interactive command line with line editing and help */
#undef XELP_ENABLE_CLI
#undef XELP_ENABLE_LINE_EDIT
#undef XELP_ENABLE_HISTORY
#undef XELP_ENABLE_ARGV
#undef XELP_ENABLE_KEY
#undef XELP_ENABLE_THR
#undef XELP_ENABLE_HELP

#define XELP_ENABLE_KEY       1
#define XELP_ENABLE_CLI       1
#define XELP_ENABLE_LINE_EDIT 1
#define XELP_ENABLE_HELP      1
EOF

cat > "$CFG_DIR/full/xelp_ovr.h" << 'EOF'
/* FULL config: all features enabled */
#undef XELP_ENABLE_CLI
#undef XELP_ENABLE_LINE_EDIT
#undef XELP_ENABLE_HISTORY
#undef XELP_ENABLE_ARGV
#undef XELP_ENABLE_KEY
#undef XELP_ENABLE_THR
#undef XELP_ENABLE_HELP

#define XELP_ENABLE_KEY       1
#define XELP_ENABLE_CLI       1
#define XELP_ENABLE_LINE_EDIT 1
#define XELP_ENABLE_HISTORY   1
#define XELP_ENABLE_ARGV      1
#define XELP_ENABLE_HELP      1
#define XELP_ENABLE_THR       1
EOF

# --- Helper: compile and return .text size ---------------------------------
# Returns size via stdout; prints nothing else to stdout.
# Uses extract_size.py for multi-format size extraction.
# Returns "unavail" if compilation failed or size could not be determined.

get_text_size() {
    local cc_cmd="$1"

    # Clean stale outputs from compilers that may ignore -o
    rm -f "$OBJ" xelp.rel xelp.asm xelp.lst xelp.sym xelp.o xelp.s 2>/dev/null

    local diag_file
    diag_file=$(mktemp)
    # Capture both stdout and stderr (cc65 writes errors to stdout)
    eval $cc_cmd >"$diag_file" 2>&1 || true

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
        echo "  DIAG: compile failed:" >> "$LOG_FILE"
        head -3 "$diag_file" >> "$LOG_FILE" 2>/dev/null
        echo "  DIAG: command was: $cc_cmd" >> "$LOG_FILE"
        rm -f "$diag_file"
        read -r ok fail < "$DIAG_COUNT_FILE"
        echo "$ok $((fail + 1))" > "$DIAG_COUNT_FILE"
        echo "unavail"
        return
    fi

    local sz
    sz=$(python3 /xelp/tools/extract_size.py "$OBJ")

    if [ "$sz" = "unavail" ]; then
        echo "  DIAG: size extraction failed for $OBJ (file exists but unrecognized format)" >> "$LOG_FILE"
        read -r ok fail < "$DIAG_COUNT_FILE"
        echo "$ok $((fail + 1))" > "$DIAG_COUNT_FILE"
    else
        read -r ok fail < "$DIAG_COUNT_FILE"
        echo "$((ok + 1)) $fail" > "$DIAG_COUNT_FILE"
    fi

    rm -f "$OBJ" "$diag_file" xelp.asm xelp.lst xelp.sym xelp.rel xelp.s 2>/dev/null
    echo "$sz"
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

    echo "" >> "$LOG_FILE"
    echo "TARGET: $label ($group)" >> "$LOG_FILE"

    local key_sz cli_sz full_sz

    echo "  CONFIG: KEY" >> "$LOG_FILE"
    key_sz=$(get_text_size  "$cc_base -DXELP_CONFIG_OVERRIDE -I $CFG_DIR/key")
    echo "  CONFIG: CLI" >> "$LOG_FILE"
    cli_sz=$(get_text_size  "$cc_base -DXELP_CONFIG_OVERRIDE -I $CFG_DIR/cli")
    echo "  CONFIG: FULL" >> "$LOG_FILE"
    full_sz=$(get_text_size "$cc_base -DXELP_CONFIG_OVERRIDE -I $CFG_DIR/full")

    echo "  RESULT: KEY=$key_sz CLI=$cli_sz FULL=$full_sz" >> "$LOG_FILE"

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
echo "All sizes are .text section bytes (GCC targets use -Os)."

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

# TODO: MCS-51 sizes inflated (~10KB CLI/FULL); needs investigation
#build_target 8 "MCS-51 (8051)" "SDCC" \
#    "sdcc -mmcs51 --model-small --opt-code-size -c $SRC $INCLUDE -o $OBJ"

build_target 8 "Z80" "SDCC" \
    "sdcc -mz80 --opt-code-size -c $SRC $INCLUDE -o $OBJ"

build_target 8 "6800 (HC08)" "SDCC" \
    "sdcc -mhc08 --stack-auto --opt-code-size -c $SRC $INCLUDE -o $OBJ"

# --- 16-bit targets --------------------------------------------------------

print_header "16-bit targets"

build_target 16 "MSP430" "msp430-gcc" \
    "msp430-gcc -c $SRC $INCLUDE -Os -Wall -o $OBJ"

build_target 16 "68HC11" "m68hc11-gcc" \
    "m68hc11-gcc -c $SRC $INCLUDE -Os -o $OBJ"

# --- 32-bit targets --------------------------------------------------------

print_header "32-bit targets"

build_target 32 "x86-32" "GCC" \
    "gcc -c $SRC $INCLUDE -Os -m32 -Wall -o $OBJ"

#build_target 32 "x86-32" "TCC" \
#    "tcc -c $SRC $INCLUDE -o $OBJ"

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
    "xtensa-esp32s3-elf-gcc -c $SRC $INCLUDE -Os -Wall -o $OBJ"

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

# --- Diagnostic summary ----------------------------------------------------
read -r DIAG_OK DIAG_FAIL < "$DIAG_COUNT_FILE"
rm -f "$DIAG_COUNT_FILE"
echo "" >> "$LOG_FILE"
echo "$SEP" >> "$LOG_FILE"
echo "SUMMARY: $DIAG_OK successful size extractions, $DIAG_FAIL failures" >> "$LOG_FILE"
echo "Diagnostic log written to $LOG_FILE"

echo ""
echo "Done. ($DIAG_OK sizes extracted, $DIAG_FAIL unavailable -- see build/crossbuild.log)"
