#!/bin/bash
# update_sizes.sh -- Update size tables in docs from build/sizes.csv
#
# Reads build/sizes.csv (produced by crossbuild.sh / compactbuilds-docker.sh)
# and patches the size tables in:
#   - README.md           (Markdown table)
#   - pages/index.html    (HTML table)
#
# Tables are delimited by a pair of <!-- Build Size Table --> markers.
# The script wipes everything between them and inserts fresh rows.
# Rows are sorted by width (8→16→32→64), then KEY size ascending.
#
# Usage:
#   bash tools/update_sizes.sh                      # default: build/sizes.csv
#   bash tools/update_sizes.sh path/to/sizes.csv    # explicit CSV path
#   bash tools/update_sizes.sh --dry-run             # preview without writing
#
# Run from the repository root directory.

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

CSV="${1:-$REPO_ROOT/build/sizes.csv}"
DRY_RUN=0
if [ "$1" = "--dry-run" ]; then
    DRY_RUN=1
    CSV="${2:-$REPO_ROOT/build/sizes.csv}"
fi

if [ ! -f "$CSV" ]; then
    echo "Error: $CSV not found."
    echo "Run 'bash tools/crossbuild.sh' first to generate it."
    exit 1
fi

echo "Reading $CSV ..."

# --- Parse CSV and sort: width ascending (8→16→32→64), then KEY size ------
# Skip header, skip FAIL rows.

SORTED=$(tail -n +2 "$CSV" | grep -v "FAIL" | sort -t',' -k2 -n -k4 -n)

if [ -z "$SORTED" ]; then
    echo "Error: no valid rows in CSV (all FAIL?)."
    exit 1
fi

ROW_COUNT=$(echo "$SORTED" | wc -l | tr -d ' ')
echo "Found $ROW_COUNT valid targets."

# --- Generate Markdown table ----------------------------------------------

MD_TABLE="| CPU | Width | Compiler | KEY (bytes) | CLI (bytes) | HIST (bytes) | SCRIPT (bytes) |
|-----|------:|----------|------------:|------------:|-------------:|---------------:|"

while IFS=',' read -r cpu width compiler key cli hist script; do
    MD_TABLE="${MD_TABLE}
| ${cpu} | ${width} | ${compiler} | ${key} | ${cli} | ${hist} | ${script} |"
done <<< "$SORTED"

# --- Generate HTML table -------------------------------------------------

HTML_TABLE='<table>
<thead><tr><th>CPU</th><th style="text-align:right">Width</th><th>Compiler</th><th style="text-align:right">KEY (bytes)</th><th style="text-align:right">CLI (bytes)</th><th style="text-align:right">HIST (bytes)</th><th style="text-align:right">SCRIPT (bytes)</th></tr></thead>
<tbody>'

while IFS=',' read -r cpu width compiler key cli hist script; do
    HTML_TABLE="${HTML_TABLE}
<tr><td>${cpu}</td><td style=\"text-align:right\">${width}</td><td>${compiler}</td><td style=\"text-align:right\">${key}</td><td style=\"text-align:right\">${cli}</td><td style=\"text-align:right\">${hist}</td><td style=\"text-align:right\">${script}</td></tr>"
done <<< "$SORTED"

HTML_TABLE="${HTML_TABLE}
</tbody>
</table>"

# --- Helper: replace content between markers ------------------------------

patch_file() {
    local file="$1"
    local replacement="$2"
    local marker='<!-- Build Size Table -->'

    if [ ! -f "$file" ]; then
        echo "  SKIP $file (not found)"
        return
    fi

    if ! grep -qF "$marker" "$file"; then
        echo "  SKIP $file (no marker)"
        return
    fi

    # Write replacement to a temp file (awk -v can't handle multi-line).
    local replfile tmpfile
    replfile=$(mktemp)
    tmpfile=$(mktemp)
    printf '%s\n' "$replacement" > "$replfile"

    awk -v m="$marker" -v rf="$replfile" '
        BEGIN { seen=0 }
        index($0, m) {
            if (seen == 0) {
                print $0
                while ((getline line < rf) > 0) print line
                close(rf)
                seen=1
                next
            } else {
                print $0
                seen=2
                next
            }
        }
        seen==1 { next }
        { print }
    ' "$file" > "$tmpfile"
    rm -f "$replfile"

    if [ $DRY_RUN -eq 1 ]; then
        echo "  DRY-RUN $file (would update)"
        rm -f "$tmpfile"
    else
        mv "$tmpfile" "$file"
        echo "  UPDATED $file"
    fi
}

# --- Patch files ----------------------------------------------------------

echo ""
echo "Patching size tables ..."
patch_file "$REPO_ROOT/README.md"         "$MD_TABLE"
patch_file "$REPO_ROOT/pages/index.html"  "$HTML_TABLE"

echo ""
echo "Done."
