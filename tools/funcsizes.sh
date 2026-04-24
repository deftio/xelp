#!/usr/bin/env bash
# tools/funcsizes.sh -- compile xelp.c with -Os for x86-32 and ARM32,
# extract per-function compiled sizes, count source lines, print a table.
set -euo pipefail

SRCDIR="$(cd "$(dirname "$0")/../src" && pwd)"
SRC="$SRCDIR/xelp.c"
BUILDDIR="$(cd "$(dirname "$0")/.." && pwd)/build"
mkdir -p "$BUILDDIR"

# ── Helpers ──────────────────────────────────────────────────────────────

# extract_sizes <object-file>
# Prints "funcname size" lines for every .text symbol.
# Works on macOS (where nm -S is not available) by computing sizes from
# sorted symbol addresses + total .text section size.
extract_sizes() {
    local obj="$1"

    # Get total .text size via size(1) -- second line, first column
    local text_size
    text_size=$(size "$obj" 2>/dev/null | awk 'NR==2{print $1}')
    if [ -z "$text_size" ]; then return; fi

    # Sorted text symbols: addr name
    local syms
    syms=$(nm -n "$obj" 2>/dev/null \
        | awk '$2~/^[tT]$/ && $3!~/^$/{
            v = 0; s = $1
            for (i = 1; i <= length(s); i++) {
                c = substr(s, i, 1)
                if      (c >= "0" && c <= "9") d = c + 0
                else if (c >= "a" && c <= "f") d = index("abcdef", c) + 9
                else if (c >= "A" && c <= "F") d = index("ABCDEF", c) + 9
                else continue
                v = v * 16 + d
            }
            printf "%d %s\n", v, $3
        }' \
        | sort -n)
    if [ -z "$syms" ]; then return; fi

    # Compute per-function size from address gaps
    local prev_addr="" prev_name=""
    local base_addr
    base_addr=$(echo "$syms" | head -1 | awk '{print $1}')
    while IFS=' ' read -r addr name; do
        if [ -n "$prev_name" ]; then
            echo "$prev_name $(( addr - prev_addr ))"
        fi
        prev_addr=$addr
        prev_name=$name
    done <<< "$syms"
    # Last function: extends to end of .text
    if [ -n "$prev_name" ]; then
        echo "$prev_name $(( base_addr + text_size - prev_addr ))"
    fi
}

# ── Source line counts ───────────────────────────────────────────────────

# Count lines per function definition by tracking brace depth.
# Skips braces inside /* */ comments and string literals.
# Outputs "funcname lines" for each top-level function body found.
count_src_lines() {
    awk '
    {
        line = $0
        # Strip characters inside /* */ comments and string literals
        # so that braces in comments/strings are not counted.
        clean = ""
        for (i = 1; i <= length(line); i++) {
            c = substr(line, i, 1)
            if (in_comment) {
                if (c == "*" && substr(line, i+1, 1) == "/") { in_comment = 0; i++ }
                continue
            }
            if (in_string) {
                if (c == "\\" ) { i++; continue }
                if (c == "\"") in_string = 0
                continue
            }
            if (in_char) {
                if (c == "\\") { i++; continue }
                if (c == "\047") in_char = 0
                continue
            }
            if (c == "/" && substr(line, i+1, 1) == "*") { in_comment = 1; i++; continue }
            if (c == "/" && substr(line, i+1, 1) == "/") break  # rest of line is comment
            if (c == "\"") { in_string = 1; continue }
            if (c == "\047") { in_char = 1; continue }
            clean = clean c
        }
    }
    !in_func && clean ~ /^[A-Za-z_]/ && clean ~ /\(/ && clean !~ /^#/ && clean !~ /;[[:space:]]*$/ && clean !~ /^typedef/ {
        n = split(clean, parts, /[[:space:](]+/)
        fname = ""
        for (i = 1; i <= n; i++) {
            if (parts[i] ~ /^[A-Za-z_][A-Za-z0-9_]*$/) {
                fname = parts[i]
                if (parts[i] ~ /^(static|void|int|char|unsigned|const|XELPRESULT|XELP|XelpBuf)$/) continue
                break
            }
        }
        if (fname == "") next
        in_func = 1
        depth = 0
        lines = 0
        func_name = fname
    }
    in_func {
        lines++
        for (i = 1; i <= length(clean); i++) {
            c = substr(clean, i, 1)
            if (c == "{") depth++
            if (c == "}") { depth--; if (depth == 0) { print func_name, lines; in_func = 0; next } }
        }
    }
    ' "$SRC"
}

# ── Compile ──────────────────────────────────────────────────────────────

X86_OBJ="$BUILDDIR/xelp_funcsizes_x86.o"
ARM_OBJ="$BUILDDIR/xelp_funcsizes_arm.o"

have_x86=0
have_arm=0

# x86-32
if gcc -m32 -Os -c "$SRC" -I"$SRCDIR" -o "$X86_OBJ" 2>/dev/null; then
    have_x86=1
else
    echo "note: gcc -m32 not available, skipping x86-32 column" >&2
fi

# ARM32
ARM_CC="${ARM_CC:-arm-none-eabi-gcc}"
if command -v "$ARM_CC" >/dev/null 2>&1; then
    if "$ARM_CC" -Os -mthumb -mcpu=cortex-m3 -c "$SRC" -I"$SRCDIR" -o "$ARM_OBJ" 2>/dev/null; then
        have_arm=1
    else
        echo "note: $ARM_CC compile failed, skipping ARM32 column" >&2
    fi
else
    echo "note: $ARM_CC not found, skipping ARM32 column" >&2
fi

if [ "$have_x86" -eq 0 ] && [ "$have_arm" -eq 0 ]; then
    echo "error: no compiler produced output" >&2
    exit 1
fi

# ── Gather data ──────────────────────────────────────────────────────────

# Collect sizes into temp files
X86_TMP=$(mktemp)
ARM_TMP=$(mktemp)
SRC_TMP=$(mktemp)
trap 'rm -f "$X86_TMP" "$ARM_TMP" "$SRC_TMP"' EXIT

[ "$have_x86" -eq 1 ] && extract_sizes "$X86_OBJ" > "$X86_TMP"
[ "$have_arm" -eq 1 ] && extract_sizes "$ARM_OBJ" > "$ARM_TMP"
count_src_lines > "$SRC_TMP"

# ── Merge & print ────────────────────────────────────────────────────────

# Strip leading underscore (macOS prepends _) and merge all data.
awk '
BEGIN {
    # Read source lines
    while ((getline line < ARGV[1]) > 0) {
        split(line, a, " ")
        src[a[1]] = a[2]
    }
    close(ARGV[1])

    # Read x86 sizes
    while ((getline line < ARGV[2]) > 0) {
        split(line, a, " ")
        name = a[1]; sub(/^_/, "", name)
        x86[name] = a[2]
        if (!(name in order)) { order[name] = ++n; names[n] = name }
    }
    close(ARGV[2])

    # Read ARM sizes
    while ((getline line < ARGV[3]) > 0) {
        split(line, a, " ")
        name = a[1]; sub(/^_/, "", name)
        arm[name] = a[2]
        if (!(name in order)) { order[name] = ++n; names[n] = name }
    }
    close(ARGV[3])

    # Determine max size for sorting (prefer x86, fall back to ARM)
    for (name in order) {
        if (name in x86) maxsz[name] = x86[name] + 0
        else if (name in arm) maxsz[name] = arm[name] + 0
        else maxsz[name] = 0
    }

    # Simple insertion sort descending by max size
    for (i = 1; i <= n; i++) sorted[i] = names[i]
    for (i = 2; i <= n; i++) {
        key = sorted[i]
        j = i - 1
        while (j >= 1 && maxsz[sorted[j]] < maxsz[key]) {
            sorted[j+1] = sorted[j]
            j--
        }
        sorted[j+1] = key
    }

    # Compute totals
    x86_total = 0; arm_total = 0; src_total = 0
    for (i = 1; i <= n; i++) {
        name = sorted[i]
        if (name in x86) x86_total += x86[name]
        if (name in arm) arm_total += arm[name]
        if (name in src) src_total += src[name]
    }

    # Print header
    printf "%-30s %8s %8s %8s\n", "Function", "SrcLines", "x86-32", "ARM32"
    printf "%-30s %8s %8s %8s\n", "------------------------------", "--------", "--------", "--------"

    # Print rows
    for (i = 1; i <= n; i++) {
        name = sorted[i]
        sl = (name in src) ? src[name] : "-"
        xv = (name in x86) ? x86[name] : "-"
        av = (name in arm) ? arm[name] : "-"
        printf "%-30s %8s %8s %8s\n", name, sl, xv, av
    }

    # Print totals
    printf "%-30s %8s %8s %8s\n", "------------------------------", "--------", "--------", "--------"
    xs = x86_total ? x86_total : "-"
    as = arm_total ? arm_total : "-"
    ss = src_total ? src_total : "-"
    printf "%-30s %8s %8s %8s\n", "TOTAL", ss, xs, as
}
' "$SRC_TMP" "$X86_TMP" "$ARM_TMP"
