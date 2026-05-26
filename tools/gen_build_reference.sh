#!/bin/bash
# gen_build_reference.sh -- Generate build/build-reference.md with measured sizes
#
# Compiles xelp.c in four configurations (KEY, CLI, HIST, SCRIPT),
# measures .text sizes and sizeof(XELP) on host, and incorporates
# cross-compiled per-platform sizes from build/sizes.csv when available.
#
# Usage:  bash tools/gen_build_reference.sh
# Output: build/build-reference.md
#
# For cross-compiled sizes: run `make prerelease` first (requires Docker),
# then `make build-ref` to include them in the document.

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
SRC="$REPO_ROOT/src/xelp.c"
HDR_DIR="$REPO_ROOT/src"
BUILD_DIR="$REPO_ROOT/build"
TMP_DIR="$REPO_ROOT/tmp/build_ref"
OUT="$REPO_ROOT/docs/build-reference.md"
CSV="$BUILD_DIR/sizes.csv"

CC="${CC:-gcc}"
CFLAGS="-Os -Wall -Wextra"

mkdir -p "$BUILD_DIR" "$TMP_DIR"

# --- Create config override headers ----------------------------------------

for prof in key cli hist script; do
    mkdir -p "$TMP_DIR/$prof"
done

cat > "$TMP_DIR/key/xelp_ovr.h" << 'EOF'
/* KEY-only config */
#undef XELP_ENABLE_CLI
#undef XELP_ENABLE_LINE_EDIT
#undef XELP_ENABLE_HISTORY
#undef XELP_ENABLE_ARGV
#undef XELP_ENABLE_KEY
#undef XELP_ENABLE_THR
#undef XELP_ENABLE_HELP
#undef XELP_ENABLE_SCRIPT

#define XELP_ENABLE_KEY       1
EOF

cat > "$TMP_DIR/cli/xelp_ovr.h" << 'EOF'
/* CLI config */
#undef XELP_ENABLE_CLI
#undef XELP_ENABLE_LINE_EDIT
#undef XELP_ENABLE_HISTORY
#undef XELP_ENABLE_ARGV
#undef XELP_ENABLE_KEY
#undef XELP_ENABLE_THR
#undef XELP_ENABLE_HELP
#undef XELP_ENABLE_SCRIPT

#define XELP_ENABLE_KEY       1
#define XELP_ENABLE_CLI       1
#define XELP_ENABLE_LINE_EDIT 1
#define XELP_ENABLE_HELP      1
EOF

cat > "$TMP_DIR/hist/xelp_ovr.h" << 'EOF'
/* HIST config */
#undef XELP_ENABLE_CLI
#undef XELP_ENABLE_LINE_EDIT
#undef XELP_ENABLE_HISTORY
#undef XELP_ENABLE_ARGV
#undef XELP_ENABLE_KEY
#undef XELP_ENABLE_THR
#undef XELP_ENABLE_HELP
#undef XELP_ENABLE_SCRIPT

#define XELP_ENABLE_KEY       1
#define XELP_ENABLE_CLI       1
#define XELP_ENABLE_LINE_EDIT 1
#define XELP_ENABLE_HELP      1
#define XELP_ENABLE_HISTORY   1
#define XELP_ENABLE_THR       1
EOF

cat > "$TMP_DIR/script/xelp_ovr.h" << 'EOF'
/* SCRIPT config */
#undef XELP_ENABLE_CLI
#undef XELP_ENABLE_LINE_EDIT
#undef XELP_ENABLE_HISTORY
#undef XELP_ENABLE_ARGV
#undef XELP_ENABLE_KEY
#undef XELP_ENABLE_THR
#undef XELP_ENABLE_HELP
#undef XELP_ENABLE_SCRIPT

#define XELP_ENABLE_KEY       1
#define XELP_ENABLE_CLI       1
#define XELP_ENABLE_LINE_EDIT 1
#define XELP_ENABLE_HELP      1
#define XELP_ENABLE_HISTORY   1
#define XELP_ENABLE_ARGV      1
#define XELP_ENABLE_THR       1
#define XELP_ENABLE_SCRIPT    1
EOF

# --- Measure sizeof(XELP) and pointer/non-pointer portions -----------------

sizeof_probe() {
    local prof="$1"
    local probe="$TMP_DIR/sizeof_${prof}.c"
    local exe="$TMP_DIR/sizeof_${prof}"

    cat > "$probe" << 'PROBE_EOF'
#include <stdio.h>
#include <stddef.h>
#include "xelp.h"

/* Count pointer-sized fields by summing sizeof of each pointer member.
   This lets us estimate sizeof(XELP) for other pointer widths:
   estimated = measured_sizeof - ptr_bytes_on_host + (num_ptrs * target_ptr_size) */
int main(void) {
    XELP x;
    unsigned long total = sizeof(XELP);
    unsigned long ptrsz = sizeof(void*);

    /* Count pointer-sized fields present in this config */
    unsigned long ptr_bytes = 0;
    ptr_bytes += sizeof(x.mpAboutMsg);
    ptr_bytes += sizeof(x.mpfOut);
    ptr_bytes += sizeof(x.mpfErr);
    ptr_bytes += sizeof(x.mpfEditModeChg);
#ifdef XELP_ENABLE_KEY
    ptr_bytes += sizeof(x.mpKeyModeFuncs);
    ptr_bytes += sizeof(x.mpfDefKey);
#endif
#ifdef XELP_ENABLE_CLI
    ptr_bytes += sizeof(x.mpCLIModeFuncs);
    ptr_bytes += sizeof(x.mpfDefCLI);
    ptr_bytes += sizeof(x.mCmdXB.s) + sizeof(x.mCmdXB.p) + sizeof(x.mCmdXB.e);
    ptr_bytes += sizeof(x.mpfBksp);
#endif
#ifdef XELP_CLI_PROMPT
    ptr_bytes += sizeof(x.mpPrompt);
#endif
#if defined(XELP_ENABLE_CLI) && defined(XELP_ENABLE_LINE_EDIT)
    ptr_bytes += sizeof(x.mCur);
#endif
#ifdef XELP_ENABLE_THR
    ptr_bytes += sizeof(x.mpfPassThru);
#endif
#ifdef XELP_ENABLE_SCRIPT
    ptr_bytes += sizeof(x.mSP);
    ptr_bytes += sizeof(x.mHP);
    ptr_bytes += sizeof(x.mpFrameArgv);
    ptr_bytes += sizeof(x.mpScriptFuncs);
    ptr_bytes += sizeof(x.mpfBreakpoint);
#endif

    /* Also count XELPKEYCODE (unsigned long -- changes with platform) */
    unsigned long long_bytes = sizeof(x.mKeyAccum);

    /* num_ptrs = ptr_bytes / ptrsz */
    printf("%lu %lu %lu %lu\n", total, ptrsz, ptr_bytes, long_bytes);
    return 0;
}
PROBE_EOF

    $CC -Os "$probe" -I "$HDR_DIR" -DXELP_CONFIG_OVERRIDE -I "$TMP_DIR/$prof" -o "$exe" 2>/dev/null
    "$exe"
    rm -f "$probe" "$exe"
}

echo "Measuring sizeof(XELP) ..."
read RAM_KEY PTR_SZ KEY_PTRBYTES KEY_LONGBYTES <<< "$(sizeof_probe key)"
read RAM_CLI _      CLI_PTRBYTES CLI_LONGBYTES <<< "$(sizeof_probe cli)"
read RAM_HIST _     HIST_PTRBYTES HIST_LONGBYTES <<< "$(sizeof_probe hist)"
read RAM_SCRIPT _   SCRIPT_PTRBYTES SCRIPT_LONGBYTES <<< "$(sizeof_probe script)"
echo "  KEY=$RAM_KEY  CLI=$RAM_CLI  HIST=$RAM_HIST  SCRIPT=$RAM_SCRIPT  (host ptr=${PTR_SZ})"

# Estimate sizeof for a target pointer width.
# estimated = measured - host_ptr_bytes - host_long_bytes + target_ptr_bytes + target_long_bytes
# This is approximate (alignment may differ) but gives a reasonable ballpark.
estimate_sizeof() {
    local measured=$1 host_ptrbytes=$2 host_longbytes=$3 target_ptrsz=$4
    local num_ptrs=$((host_ptrbytes / PTR_SZ))
    local target_ptrbytes=$((num_ptrs * target_ptrsz))
    # unsigned long: 4 bytes on 8/16/32-bit, 8 bytes on 64-bit host
    local target_longbytes=4
    echo $(( measured - host_ptrbytes - host_longbytes + target_ptrbytes + target_longbytes ))
}

# Compute estimated sizeof for 32-bit and 16-bit targets
RAM32_KEY=$(estimate_sizeof $RAM_KEY $KEY_PTRBYTES $KEY_LONGBYTES 4)
RAM32_CLI=$(estimate_sizeof $RAM_CLI $CLI_PTRBYTES $CLI_LONGBYTES 4)
RAM32_HIST=$(estimate_sizeof $RAM_HIST $HIST_PTRBYTES $HIST_LONGBYTES 4)
RAM32_SCRIPT=$(estimate_sizeof $RAM_SCRIPT $SCRIPT_PTRBYTES $SCRIPT_LONGBYTES 4)

RAM16_KEY=$(estimate_sizeof $RAM_KEY $KEY_PTRBYTES $KEY_LONGBYTES 2)
RAM16_CLI=$(estimate_sizeof $RAM_CLI $CLI_PTRBYTES $CLI_LONGBYTES 2)
RAM16_HIST=$(estimate_sizeof $RAM_HIST $HIST_PTRBYTES $HIST_LONGBYTES 2)
RAM16_SCRIPT=$(estimate_sizeof $RAM_SCRIPT $SCRIPT_PTRBYTES $SCRIPT_LONGBYTES 2)

echo "  32-bit est: KEY=$RAM32_KEY  CLI=$RAM32_CLI  HIST=$RAM32_HIST  SCRIPT=$RAM32_SCRIPT"
echo "  16-bit est: KEY=$RAM16_KEY  CLI=$RAM16_CLI  HIST=$RAM16_HIST  SCRIPT=$RAM16_SCRIPT"

# --- Collect compiler info --------------------------------------------------

CC_VERSION=$($CC --version 2>/dev/null | head -1)
HOST_ARCH=$(uname -m)
HOST_OS=$(uname -s)
HOST_BITS=$(getconf LONG_BIT)
GEN_DATE=$(date -u '+%Y-%m-%d %H:%M UTC')

# --- Extract version from xelp.h -------------------------------------------

XELP_VER_HEX=$(grep '#define XELP_VERSION' "$HDR_DIR/xelp.h" | grep -oE '0x[0-9A-Fa-f]+UL' | sed 's/UL$//')
if [ -n "$XELP_VER_HEX" ]; then
    VER_NUM=$((XELP_VER_HEX))
    VER_MAJOR=$(( (VER_NUM >> 16) & 0xFF ))
    VER_MINOR=$(( (VER_NUM >>  8) & 0xFF ))
    VER_PATCH=$((  VER_NUM        & 0xFF ))
    XELP_VER="${VER_MAJOR}.${VER_MINOR}.${VER_PATCH}"
else
    XELP_VER="unknown"
fi

# --- Build cross-compiled code size table from CSV --------------------------

CROSS_CODE_TABLE=""
if [ -f "$CSV" ]; then
    echo "Reading cross-compiled sizes from $CSV ..."
    SORTED=$(tail -n +2 "$CSV" | grep -v "FAIL" | grep -v "unavail" | sort -t',' -k2 -n -k4 -n)
    if [ -n "$SORTED" ]; then
        ROW_COUNT=$(echo "$SORTED" | wc -l | tr -d ' ')
        echo "  $ROW_COUNT targets found."
        CROSS_CODE_TABLE="| Platform | Bits | Compiler | KEY | CLI | HIST | SCRIPT |
|----------|-----:|----------|----:|----:|-----:|-------:|"
        while IFS=',' read -r cpu width compiler key cli hist script; do
            CROSS_CODE_TABLE="${CROSS_CODE_TABLE}
| ${cpu} | ${width} | ${compiler} | ${key} | ${cli} | ${hist} | ${script} |"
        done <<< "$SORTED"
    fi
fi

# --- Emit build-reference.md -----------------------------------------------

echo "Writing $OUT ..."

# Build the code size section
if [ -n "$CROSS_CODE_TABLE" ]; then
    CODE_SIZE_SECTION="## Code Size (.text bytes, \`-Os\`)

${CROSS_CODE_TABLE}

All targets compiled with \`-Os\`. Sizes are \`.text\` section only (no data/BSS).
Run \`make prerelease\` to regenerate (requires Docker)."
else
    CODE_SIZE_SECTION="## Code Size (.text bytes, \`-Os\`)

No cross-compiled data. Run \`make prerelease\` (requires Docker) to cross-compile
for AVR, MSP430, ARM Thumb, ESP32, RISC-V, MIPS, and more."
fi

cat > "$OUT" << DOCEOF
# xelp Build Reference

> **Auto-generated** on ${GEN_DATE}
> xelp ${XELP_VER} | ${CC_VERSION} | ${HOST_OS} ${HOST_ARCH}
>
> Regenerate: \`make build-ref\`

---

## Build Profiles

Four ready-made profiles. Each is a set of \`#define\` flags that control which
features are compiled in. Unused features are stripped at compile time.

All flags are independent -- mix and match freely. These profiles are convenient
bundles, not hard requirements. For example, \`XELP_ENABLE_THR\` can be added to
any profile, or \`XELP_ENABLE_SCRIPT\` can be used without \`XELP_ENABLE_HISTORY\`.

| Profile | Flags | Description |
|---------|-------|-------------|
| **KEY** | \`XELP_ENABLE_KEY\` | Single-keypress dispatch only. Menus, debug jigs, minimal footprint. |
| **CLI** | \`KEY\` + \`CLI\` + \`LINE_EDIT\` + \`HELP\` | Interactive command line with cursor editing and help. |
| **HIST** | CLI + \`HISTORY\` + \`THR\` | CLI plus command history (UP/DOWN recall) and pass-through mode. |
| **SCRIPT** | HIST + \`SCRIPT\` + \`ARGV\` | All features including the XelpScript engine. |

---

## Feature Matrix

Flags marked \`*\` have no dependencies and can be added to any profile.

| Feature | KEY | CLI | HIST | SCRIPT |
|---------|:---:|:---:|:----:|:------:|
| Single-key dispatch (menus) | Y | Y | Y | Y |
| Command line prompt + ENTER | -- | Y | Y | Y |
| Cursor movement (left/right, Home/End) | -- | Y | Y | Y |
| Insert-at-cursor, Delete | -- | Y | Y | Y |
| Multi-byte ANSI key recognition | -- | Y | Y | Y |
| Command dispatch (tokenizer, function tables) | -- | Y | Y | Y |
| Built-in help listing \`*\` | -- | Y | Y | Y |
| Script execution (semicolons, newlines, comments) | -- | Y | Y | Y |
| Quoted strings with escape sequences | -- | Y | Y | Y |
| Command history (UP/DOWN arrow) | -- | -- | Y | Y |
| THR pass-through mode \`*\` | -- | -- | Y | Y |
| Script variables (\`_set\`, \`\$var\`) | -- | -- | -- | Y |
| Math builtins (\`_add\`, \`_mul\`, etc.) | -- | -- | -- | Y |
| Comparison + logic (\`_eq\`, \`_lt\`, \`_not\`) | -- | -- | -- | Y |
| Conditionals (\`_if\`/\`_then\`/\`_else\`) | -- | -- | -- | Y |
| Labels + jumps (\`_goto\`, \`_next\`) | -- | -- | -- | Y |
| Script functions (\`_func\`, \`_return\`, \`@1\`/\`@2\`) | -- | -- | -- | Y |
| C-registered ROM script functions | -- | -- | -- | Y |
| C interop (\`XelpCallProc\`) | -- | -- | -- | Y |
| Register read/write (\`_mr\`) | -- | -- | -- | Y |
| Parenthesized subexpressions | -- | -- | -- | Y |
| Breakpoint safety callback | -- | -- | -- | Y |

---

${CODE_SIZE_SECTION}

---

## RAM Per Instance (\`sizeof(XELP)\`, default buffer sizes)

With \`XELP_CMDBUFSZ=64\`, \`XELP_HIST_DEPTH=4\`, \`XELP_SCRIPT_ARENA_SZ=sizeof(int)*256\`.

| Platform class | KEY | CLI | HIST | SCRIPT |
|----------------|----:|----:|-----:|-------:|
| 16-bit (MSP430, AVR) | ~${RAM16_KEY} | ~${RAM16_CLI} | ~${RAM16_HIST} | ~${RAM16_SCRIPT} |
| 32-bit (ARM Thumb, ESP32, RISC-V) | ~${RAM32_KEY} | ~${RAM32_CLI} | ~${RAM32_HIST} | ~${RAM32_SCRIPT} |
| 64-bit (x86-64, AArch64) | ${RAM_KEY} | ${RAM_CLI} | ${RAM_HIST} | ${RAM_SCRIPT} |

64-bit row is measured on ${HOST_OS} ${HOST_ARCH}. 32-bit and 16-bit rows are
estimated from the measured value by adjusting for pointer width (each pointer
field is ${PTR_SZ} bytes on host vs 4 or 2 bytes on the target). Actual values
may vary slightly due to alignment differences.

Most of the RAM is fixed-size arrays (\`mCmdMsgBuf\`, \`mHistBuf\`, \`mArena\`) that
don't change with word size. The pointer-dependent portion is small relative
to the buffers.

### What Dominates RAM

| Component | Bytes | Present in |
|-----------|------:|-----------|
| \`mArena[]\` (script engine arena) | 512-2048 | SCRIPT only (scales with word size) |
| \`mHistBuf[4][64]\` + \`mHistSaved[64]\` (history ring) | 324 | HIST, SCRIPT |
| \`mCmdMsgBuf[64]\` + \`mArgvBuf[64]\` (CLI buffers) | 128 | CLI, HIST, SCRIPT |
| \`mGotoLabel[16]\` (script goto target) | 16 | SCRIPT only |
| \`mR[4]\` (return registers) | 16 | All |
| Pointers (function tables, callbacks, XelpBuf) | 12-160 | varies by config and word size |
| Scalar state (mode, counters, flags) | ~10 | All |

---

## RAM Tuning Knobs

| Parameter | Default | Effect |
|-----------|--------:|--------|
| \`XELP_CMDBUFSZ\` | 64 | CLI input buffer + each history slot. Reducing to 32 saves 32 B x (1 + HIST_DEPTH + 1) buffers. |
| \`XELP_HIST_DEPTH\` | 4 | History ring slots. Reducing to 2 saves \`2 * XELP_CMDBUFSZ\` (~128 B). |
| \`XELP_SCRIPT_ARENA_SZ\` | \`sizeof(int)*256\` | Script engine arena. 512 on 16-bit, 1024 on 32-bit, 2048 on 64-bit. Override with \`-DXELP_SCRIPT_ARENA_SZ=N\`. |
| \`XELP_ARGVBUFSZ\` | 64 | Tokenization scratch buffer. Defaults to \`XELP_CMDBUFSZ\`. Argv capacity is derived: \`XELP_ARGVBUFSZ / sizeof(ptr)\`. |
| \`XELP_REGS_SZ\` | 4 | Return registers (min 4). Each is \`sizeof(XELPREG)\` bytes. |
| \`XELPREG\` | \`int\` | Register type. Use \`short\` on 8-bit targets to save space. |

---

## Compile-Time Flags

### Feature Flags

| Flag | What it enables | Requires | Code cost |
|------|----------------|----------|----------|
| \`XELP_ENABLE_KEY\` | Single key press dispatch (menus) | -- | ~200-500 B |
| \`XELP_ENABLE_CLI\` | Command line prompt, tokenizer, command dispatch | -- | ~1.5-2.5 KB |
| \`XELP_ENABLE_LINE_EDIT\` | Cursor movement, insert-at-cursor, Delete, ANSI keys | \`CLI\` | ~800-1000 B |
| \`XELP_ENABLE_HISTORY\` | Command history (UP/DOWN arrow recall) | \`CLI\` + \`LINE_EDIT\` | ~420 B |
| \`XELP_ENABLE_THR\` | Pass-through mode (redirect keys to peripheral) | **none** | ~50-125 B |
| \`XELP_ENABLE_HELP\` | Built-in help command listing | **none** | ~180-350 B |
| \`XELP_ENABLE_SCRIPT\` | XelpScript: variables, math, conditionals, labels, functions | \`CLI\` | ~1-2 KB |
| \`XELP_ENABLE_FULL\` | Shorthand: KEY + CLI + THR + HELP (no LINE_EDIT, HISTORY, SCRIPT) | -- | combined |

### Dependency Rules

Unmet dependencies are silently disabled (no compile errors):

- \`LINE_EDIT\` requires \`CLI\`
- \`HISTORY\` requires \`CLI\` + \`LINE_EDIT\`
- \`SCRIPT\` requires \`CLI\`
- \`KEY\`, \`THR\`, \`HELP\` -- **no dependencies**, work alone or with any profile

### Key Mappings

| Define | Default | Purpose |
|--------|---------|---------|
| \`XELPKEY_CLI\` | CTRL-P (0x10) | Enter CLI mode |
| \`XELPKEY_KEY\` | ESC (0x1B) | Enter KEY mode |
| \`XELPKEY_THR\` | CTRL-T (0x14) | Enter THR mode |

### Escape Characters

| Define | Default | Purpose |
|--------|---------|---------|
| \`XELP_CLI_ESC\` | \` (backtick) | Escape at command line / in scripts |
| \`XELP_QUO_ESC\` | \`\\\\\` (backslash) | Escape inside quoted strings |
| \`XELP_ESC_MAP\` | \`"n\\x0A" "t\\x09" ""\` | Quoted string escape map |

### Enter Key Detection

| Define | Default | Purpose |
|--------|---------|---------|
| \`XELP_ENTER_LF\` | 1 | Accept LF (\`\\n\`) as ENTER |
| \`XELP_ENTER_CR\` | 1 | Accept CR (\`\\r\`) as ENTER |

Both enabled by default. CR+LF pairs are coalesced.

### Prompt and Help

| Define | Default | Purpose |
|--------|---------|---------|
| \`XELP_CLI_PROMPT\` | \`"xelp>"\` | CLI prompt string |
| \`XELP_HELP_KEY_STR\` | \`"\\nKey functions\\n"\` | Help section header for KEY commands |
| \`XELP_HELP_CLI_STR\` | \`"\\nCLI functions\\n"\` | Help section header for CLI commands |
| \`XELP_HELP_ABT_STR\` | \`(ths->mpAboutMsg)\` | About message at top of help |

For per-instance prompts: \`#define XELP_CLI_PROMPT (ths->mpPrompt)\` then
\`XELP_SET_VAL_CLI_PROMPT(myXelp, "ser1>")\` at runtime.

---

## Config Override

Customize the build without modifying source files:

1. Pass \`-DXELP_CONFIG_OVERRIDE\` in compiler flags
2. Create \`xelp_ovr.h\` in your include path
3. \`#undef\` to disable, \`#undef\` + \`#define\` to change

\`\`\`c
/* xelp_ovr.h -- lean build for ATtiny (KEY only) */
#undef XELP_ENABLE_CLI
#undef XELP_ENABLE_LINE_EDIT
#undef XELP_ENABLE_HISTORY
#undef XELP_ENABLE_THR
#undef XELP_ENABLE_HELP
#undef XELP_ENABLE_SCRIPT
\`\`\`

\`\`\`c
/* xelp_ovr.h -- CLI + THR, no history, no scripting */
#undef XELP_ENABLE_CLI
#undef XELP_ENABLE_LINE_EDIT
#undef XELP_ENABLE_HISTORY
#undef XELP_ENABLE_ARGV
#undef XELP_ENABLE_KEY
#undef XELP_ENABLE_THR
#undef XELP_ENABLE_HELP
#undef XELP_ENABLE_SCRIPT

#define XELP_ENABLE_KEY       1
#define XELP_ENABLE_CLI       1
#define XELP_ENABLE_LINE_EDIT 1
#define XELP_ENABLE_HELP      1
#define XELP_ENABLE_THR       1
\`\`\`

---

*Generated by \`tools/gen_build_reference.sh\`*
DOCEOF

echo "Done. Output: $OUT"
