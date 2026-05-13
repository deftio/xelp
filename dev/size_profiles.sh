#!/bin/bash
# size_profiles.sh -- Report xelp compiled size across feature profiles.
#
# Runs inside the xelp-crossbuild docker image to get ARM Cortex-M0
# (Thumb) sizes. Falls back to host gcc (x86-64) if docker is unavailable.
#
# Usage:
#   ./dev/size_profiles.sh          (from repo root)
#   make sizes                      (if wired into Makefile)

set -e

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OVR_NAME="xelp_ovr.h"

# --- Detect build environment -------------------------------------------

DOCKER_IMG="xelp-crossbuild:latest"
USE_DOCKER=0

if docker image inspect "$DOCKER_IMG" >/dev/null 2>&1; then
    USE_DOCKER=1
fi

# --- Build function -----------------------------------------------------

# Writes an override header, compiles, and prints the text size.
# Args: label flag [flag ...]
build() {
    local label="$1"; shift
    local ovrdir="/tmp/xelp_sizes_$$"
    mkdir -p "$ovrdir"
    local ovr="$ovrdir/$OVR_NAME"

    # Undef all feature flags so #ifndef guards in xelpcfg.h don't shadow
    # the selective re-enables below.
    cat > "$ovr" << 'HEADER'
#undef XELP_ENABLE_CLI
#undef XELP_ENABLE_LINE_EDIT
#undef XELP_ENABLE_HISTORY
#undef XELP_ENABLE_KEY
#undef XELP_ENABLE_THR
#undef XELP_ENABLE_HELP
#undef XELP_ENABLE_ARGV
HEADER

    local has_help=0 has_cli=0
    for flag in "$@"; do
        echo "#define $flag 1" >> "$ovr"
        [ "$flag" = "XELP_ENABLE_HELP" ] && has_help=1
        [ "$flag" = "XELP_ENABLE_CLI" ]  && has_cli=1
    done

    local sz_arm="" sz_host=""
    local obj="/tmp/xelp_size_$$.o"

    if [ $USE_DOCKER -eq 1 ]; then
        sz_arm=$(docker run --rm --platform linux/amd64 \
            -v "$REPO_ROOT:/work:ro" \
            -v "$ovrdir:/ovr:ro" \
            "$DOCKER_IMG" sh -c "
                arm-none-eabi-gcc -Os -mcpu=cortex-m0 -mthumb -c \
                    -I/work/src -I/ovr -DXELP_CONFIG_OVERRIDE \
                    /work/src/xelp.c -o /tmp/out.o 2>/dev/null && \
                arm-none-eabi-size /tmp/out.o | awk 'NR==2{print \$1}'
            " 2>/dev/null)
    fi

    # Host gcc (always available, provides x86-64 or aarch64 baseline)
    gcc -Os -c -I"$REPO_ROOT/src" -I"$ovrdir" -DXELP_CONFIG_OVERRIDE \
        "$REPO_ROOT/src/xelp.c" -o "$obj" 2>/dev/null
    sz_host=$(size "$obj" 2>/dev/null | awk 'NR==2{print $1}')
    rm -f "$obj"

    if [ $USE_DOCKER -eq 1 ]; then
        printf "%6s  %6s  %s\n" "${sz_arm:---}" "${sz_host:---}" "$label"
    else
        printf "%6s  %s\n" "${sz_host:---}" "$label"
    fi

    rm -rf "$ovrdir"
}

# --- Print table --------------------------------------------------------

echo ""
echo "xelp compiled .text sizes (bytes, -Os)"
echo ""

if [ $USE_DOCKER -eq 1 ]; then
    printf "%6s  %6s  %s\n" "ARM-M0" "Host" "Profile"
    printf "%6s  %6s  %s\n" "------" "------" "-------"
else
    echo "(docker image '$DOCKER_IMG' not found -- showing host gcc only)"
    echo ""
    printf "%6s  %s\n" "Host" "Profile"
    printf "%6s  %s\n" "------" "-------"
fi

build "1.  KEY only"                  XELP_ENABLE_KEY
build "2.  CLI only"                  XELP_ENABLE_CLI
build "3.  CLI + help"                XELP_ENABLE_CLI XELP_ENABLE_HELP
build "4.  CLI + key"                 XELP_ENABLE_CLI XELP_ENABLE_KEY
build "5.  CLI + help + key"          XELP_ENABLE_CLI XELP_ENABLE_HELP XELP_ENABLE_KEY
build "6.  CLI + help + key + thru"   XELP_ENABLE_CLI XELP_ENABLE_HELP XELP_ENABLE_KEY XELP_ENABLE_THR
build "7.  CLI + line edit"           XELP_ENABLE_CLI XELP_ENABLE_LINE_EDIT
build "8.  CLI + LE + help"           XELP_ENABLE_CLI XELP_ENABLE_LINE_EDIT XELP_ENABLE_HELP
build "9.  CLI + LE + help + key"     XELP_ENABLE_CLI XELP_ENABLE_LINE_EDIT XELP_ENABLE_HELP XELP_ENABLE_KEY
build "10. Full"                      XELP_ENABLE_CLI XELP_ENABLE_LINE_EDIT XELP_ENABLE_HELP XELP_ENABLE_KEY XELP_ENABLE_THR
build "11. Full + argv"               XELP_ENABLE_CLI XELP_ENABLE_LINE_EDIT XELP_ENABLE_HELP XELP_ENABLE_KEY XELP_ENABLE_THR XELP_ENABLE_ARGV
build "12. Full + history"            XELP_ENABLE_CLI XELP_ENABLE_LINE_EDIT XELP_ENABLE_HELP XELP_ENABLE_KEY XELP_ENABLE_THR XELP_ENABLE_HISTORY
build "13. Full + argv + history"     XELP_ENABLE_CLI XELP_ENABLE_LINE_EDIT XELP_ENABLE_HELP XELP_ENABLE_KEY XELP_ENABLE_THR XELP_ENABLE_ARGV XELP_ENABLE_HISTORY

echo ""
