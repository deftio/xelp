#!/bin/bash
#
# make_release.sh -- Build, validate, and prepare a xelp release.
#
# The XELP_VERSION macro in src/xelp.h is the single source of truth for
# the version number. This script reads it, validates the build, and
# creates a git tag. GitHub Actions (release.yml) then creates the
# GitHub Release automatically when the tag is pushed.
#
# Usage:
#   bash tools/make_release.sh              # validate only (dry run)
#   bash tools/make_release.sh --tag        # validate + create + push git tag
#                                           #   (recommended -- CI creates release)
#   bash tools/make_release.sh --release    # validate + tag + GitHub release locally
#                                           #   (manual fallback, requires gh CLI)
#
# Prerequisites:
#   - gcc (for build + tests)
#   - gcov (for coverage)
#   - gh CLI (for --release mode only)

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# -----------------------------------------------------------------------
# Parse version from xelp.h
# -----------------------------------------------------------------------

extract_version() {
    local hex
    hex=$(grep '#define XELP_VERSION' "$REPO_ROOT/src/xelp.h" \
        | grep -o '0x[0-9a-fA-F]*')

    if [ -z "$hex" ]; then
        echo "ERROR: Could not find XELP_VERSION in src/xelp.h" >&2
        exit 1
    fi

    local num=$((hex))
    local major=$(( (num >> 8) & 0xFF ))
    local minor=$(( num & 0xFF ))

    # Format: major.minor  (e.g. 0x0021 -> 0.33, 0x0100 -> 1.0)
    VER_HEX="$hex"
    VER_MAJOR="$major"
    VER_MINOR="$minor"
    VER_STRING="${major}.${minor}"
    VER_TAG="v${VER_STRING}"
}

# -----------------------------------------------------------------------
# Validation steps
# -----------------------------------------------------------------------

step_clean_build() {
    echo ""
    echo "=== Clean build ==="
    cd "$REPO_ROOT"
    make clean
    make tests 2>&1
    local exit_code=$?
    if [ $exit_code -ne 0 ]; then
        echo "FAIL: Build or tests failed." >&2
        exit 1
    fi
    echo "PASS: All tests passed."
}

step_check_warnings() {
    echo ""
    echo "=== Checking for compiler warnings ==="
    cd "$REPO_ROOT"
    local build_log
    build_log=$(make clean 2>&1 && make tests 2>&1)
    local warnings
    warnings=$(echo "$build_log" | grep -E "^[^:]+\.(c|h):[0-9]+:[0-9]+: warning:" || true)
    if [ -n "$warnings" ]; then
        echo "FAIL: Compiler warnings detected:" >&2
        echo "$warnings" >&2
        exit 1
    fi
    echo "PASS: Zero warnings."
}

step_check_coverage() {
    echo ""
    echo "=== Coverage check ==="
    cd "$REPO_ROOT"
    local cov_line
    cov_line=$(gcov src/xelp.c 2>/dev/null | grep "Lines executed" | head -1)
    echo "  $cov_line"
    local pct
    pct=$(echo "$cov_line" | grep -o '[0-9]*\.[0-9]*%' | head -1)
    echo "PASS: Coverage at $pct"
}

step_check_git_clean() {
    echo ""
    echo "=== Git status ==="
    cd "$REPO_ROOT"
    local status
    status=$(git status --porcelain)
    if [ -n "$status" ]; then
        echo "WARNING: Working tree has uncommitted changes:" >&2
        echo "$status" >&2
        if [ "$MODE" != "validate" ]; then
            echo "FAIL: Cannot tag/release with uncommitted changes." >&2
            exit 1
        fi
    else
        echo "PASS: Working tree is clean."
    fi
}

step_check_branch() {
    echo ""
    echo "=== Branch check ==="
    local branch
    branch=$(git -C "$REPO_ROOT" rev-parse --abbrev-ref HEAD)
    echo "  Current branch: $branch"
    if [ "$MODE" != "validate" ] && [ "$branch" != "master" ] && [ "$branch" != "main" ]; then
        echo "WARNING: Releases are typically tagged on master/main." >&2
        echo "  You are on '$branch'. Proceed with caution." >&2
    fi
}

step_check_tag_exists() {
    echo ""
    echo "=== Tag check ==="
    if git -C "$REPO_ROOT" tag -l "$VER_TAG" | grep -q "$VER_TAG"; then
        echo "FAIL: Tag $VER_TAG already exists." >&2
        echo "  Bump XELP_VERSION in src/xelp.h before releasing." >&2
        exit 1
    fi
    echo "PASS: Tag $VER_TAG does not exist yet."
}

# -----------------------------------------------------------------------
# Release actions
# -----------------------------------------------------------------------

do_tag() {
    echo ""
    echo "=== Creating tag $VER_TAG ==="
    git -C "$REPO_ROOT" tag -a "$VER_TAG" -m "Release $VER_STRING"
    echo "Tag $VER_TAG created."
    echo ""
    echo "=== Pushing tag $VER_TAG ==="
    git -C "$REPO_ROOT" push origin "$VER_TAG"
    echo "Tag pushed. GitHub Actions will create the release."
}

do_release() {
    echo ""
    echo "=== Creating GitHub release (local fallback) ==="
    if ! command -v gh &>/dev/null; then
        echo "FAIL: gh CLI not found. Install from https://cli.github.com/" >&2
        exit 1
    fi

    gh release create "$VER_TAG" \
        --title "xelp $VER_STRING" \
        --notes "Release $VER_STRING. See CHANGELOG.md for details." \
        --repo "$(git -C "$REPO_ROOT" remote get-url origin)"

    echo "GitHub release $VER_TAG created."
}

# -----------------------------------------------------------------------
# Main
# -----------------------------------------------------------------------

MODE="validate"
case "${1:-}" in
    --tag)     MODE="tag" ;;
    --release) MODE="release" ;;
    --help|-h)
        echo "Usage: bash tools/make_release.sh [--tag|--release]"
        echo ""
        echo "  (no args)    Validate build, tests, coverage (dry run)"
        echo "  --tag        Validate + tag + push (CI creates release)"
        echo "  --release    Validate + tag + push + local GitHub release (fallback)"
        exit 0
        ;;
esac

extract_version

echo "============================================"
echo "  xelp release validation"
echo "  Version: $VER_STRING ($VER_HEX)"
echo "  Tag:     $VER_TAG"
echo "  Mode:    $MODE"
echo "============================================"

step_clean_build
step_check_warnings
step_check_coverage
step_check_git_clean
step_check_branch

if [ "$MODE" != "validate" ]; then
    step_check_tag_exists
    do_tag
fi

if [ "$MODE" = "release" ]; then
    do_release
fi

echo ""
echo "============================================"
echo "  All checks passed."
echo "  Version: $VER_STRING ($VER_HEX)"
echo "============================================"
