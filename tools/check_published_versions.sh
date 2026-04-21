#!/usr/bin/env bash
# check_published_versions.sh -- Print the latest published xelp version
# on each distribution platform.
#
# Requirements: curl, jq, gh (GitHub CLI)
#
# Usage:
#   bash tools/check_published_versions.sh

set -euo pipefail

REPO="deftio/xelp"
COMPONENT="deftio/xelp"

# Local info
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
branch=$(git -C "$REPO_ROOT" branch --show-current 2>/dev/null || echo "n/a")
local_ver="n/a"
if command -v gcc >/dev/null 2>&1; then
  tmpbin=$(mktemp /tmp/xelp_ver_XXXXXX)
  if gcc "$REPO_ROOT/tools/extract_version.c" -I"$REPO_ROOT/src" -o "$tmpbin" 2>/dev/null; then
    local_ver=$("$tmpbin" /dev/stdout | grep '^version:' | sed 's/version: "//;s/"//')
  fi
  rm -f "$tmpbin"
fi

printf "%-24s %s\n" "Branch" "$branch"
printf "%-24s %s\n" "Local (xelp.h)" "$local_ver"
echo ""
printf "%-24s %s\n" "Platform" "Latest Version"
printf "%-24s %s\n" "--------" "--------------"

# 1. GitHub Releases
gh_ver=$(gh release view --repo "$REPO" --json tagName -q '.tagName' 2>/dev/null || echo "n/a")
printf "%-24s %s\n" "GitHub Releases" "$gh_ver"

# 2. Arduino Library Manager
arduino_ver=$(curl -sf "https://downloads.arduino.cc/libraries/library_index.json.gz" \
  | gunzip \
  | jq -r '[.libraries[] | select(.name == "xelp")] | sort_by(.version) | last | .version // "n/a"' \
  2>/dev/null || echo "n/a")
printf "%-24s %s\n" "Arduino Library" "$arduino_ver"

# 3. ESP-IDF Component Registry
idf_ver=$(curl -sf "https://components.espressif.com/api/components/$COMPONENT" \
  | jq -r '.versions[0].version // "n/a"' \
  2>/dev/null || echo "n/a")
printf "%-24s %s\n" "ESP-IDF Components" "$idf_ver"

# 4. PlatformIO Registry
pio_ver=$(curl -sf "https://api.registry.platformio.org/v3/packages/deftio/library/xelp" \
  | jq -r '.version.name // "n/a"' \
  2>/dev/null || echo "n/a")
printf "%-24s %s\n" "PlatformIO Registry" "$pio_ver"
