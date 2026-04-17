#!/bin/bash
#
# make_release.sh -- Guided release pipeline for xelp.
#
# Walks through every step from local validation to published GitHub
# Release, pausing for confirmation before anything visible to others.
# The XELP_VERSION macro in src/xelp.h is the single source of truth;
# the version is read via the C preprocessor (tools/extract_version.c),
# not regex.
#
# Usage:
#   bash tools/make_release.sh                 # full guided release
#   bash tools/make_release.sh --validate      # local validation only
#   bash tools/make_release.sh --release-local # full flow, local GH release fallback
#
# See tools/make-release.md for detailed documentation.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$REPO_ROOT"

STEP=0
MODE="full"
BRANCH=""
ON_MASTER=false

# -----------------------------------------------------------------------
# Helpers
# -----------------------------------------------------------------------

step_header() {
    STEP=$((STEP + 1))
    echo ""
    echo "===[ Step $STEP: $1 ]==="
}

confirm() {
    local prompt="${1:-Continue?}"
    echo ""
    read -r -p "  $prompt [Y/n] " answer
    case "${answer:-y}" in
        [Yy]*) return 0 ;;
        *)
            echo "  Aborted by user."
            exit 1
            ;;
    esac
}

fail() {
    echo "  FAIL: $1" >&2
    exit 1
}

pass() {
    echo "  PASS: $1"
}

# -----------------------------------------------------------------------
# Step 1: Extract version
# -----------------------------------------------------------------------

do_extract_version() {
    step_header "Extract version from xelp.h"

    mkdir -p "$REPO_ROOT/build"
    gcc "$REPO_ROOT/tools/extract_version.c" \
        -I"$REPO_ROOT/src" \
        -o "$REPO_ROOT/build/extract_version" \
        || fail "Could not compile extract_version.c"

    "$REPO_ROOT/build/extract_version" "$REPO_ROOT/build/xelp_version.yaml" \
        || fail "extract_version failed"

    VER_HEX=$(grep '^version_hex:' build/xelp_version.yaml | cut -d'"' -f2)
    VER_STRING=$(grep '^version:' build/xelp_version.yaml | cut -d'"' -f2)
    VER_TAG=$(grep '^tag:' build/xelp_version.yaml | cut -d'"' -f2)

    [ -n "$VER_STRING" ] || fail "No version string produced"

    echo "  Version: $VER_STRING ($VER_HEX)"
    echo "  Tag:     $VER_TAG"
    cat build/xelp_version.yaml
}

# -----------------------------------------------------------------------
# Step 2: Local validation
# -----------------------------------------------------------------------

do_validate() {
    step_header "Local validation (build, tests, warnings, coverage)"

    echo "  --- Clean build + tests ---"
    make clean >/dev/null 2>&1
    make tests 2>&1
    pass "All tests passed."

    echo ""
    echo "  --- Zero-warning check ---"
    local build_log warnings
    build_log=$(make clean 2>&1 && make tests 2>&1)
    warnings=$(echo "$build_log" | grep -E "^[^:]+\.(c|h):[0-9]+:[0-9]+: warning:" || true)
    if [ -n "$warnings" ]; then
        echo "$warnings" >&2
        fail "Compiler warnings detected."
    fi
    pass "Zero warnings."

    echo ""
    echo "  --- Coverage ---"
    local cov_line pct
    cov_line=$(gcov src/xelp.c 2>/dev/null | grep "Lines executed" | head -1)
    pct=$(echo "$cov_line" | grep -o '[0-9]*\.[0-9]*%' | head -1)
    pass "Coverage: $pct"
}

# -----------------------------------------------------------------------
# Step 3: Check git state
# -----------------------------------------------------------------------

do_check_git() {
    step_header "Check git state"

    BRANCH=$(git rev-parse --abbrev-ref HEAD)
    echo "  Branch: $BRANCH"

    if [ "$BRANCH" = "master" ] || [ "$BRANCH" = "main" ]; then
        ON_MASTER=true
        echo "  Already on $BRANCH -- will skip PR steps."
    fi

    # Tag collision check
    if git tag -l "$VER_TAG" | grep -q "$VER_TAG"; then
        fail "Tag $VER_TAG already exists. Bump XELP_VERSION in src/xelp.h first."
    fi
    pass "Tag $VER_TAG does not exist yet."

    # Working tree
    local status
    status=$(git status --porcelain)
    if [ -n "$status" ]; then
        echo ""
        echo "  Uncommitted changes:"
        git status --short
        if [ "$MODE" = "validate" ]; then
            echo "  (validate mode -- continuing with warning)"
            return 0
        fi
        echo ""
        echo "  You need a clean tree before releasing."
        echo "  Options: commit your changes, stash them, or abort."
        confirm "Abort so you can commit?"
        exit 1
    else
        pass "Working tree is clean."
    fi
}

# -----------------------------------------------------------------------
# Step 4: Push branch
# -----------------------------------------------------------------------

do_push_branch() {
    if $ON_MASTER; then return 0; fi

    step_header "Push branch '$BRANCH' to origin"

    local tracking
    tracking=$(git rev-parse --abbrev-ref --symbolic-full-name "@{u}" 2>/dev/null || true)
    if [ -n "$tracking" ]; then
        local ahead
        ahead=$(git rev-list --count "@{u}..HEAD" 2>/dev/null || echo "0")
        if [ "$ahead" -eq 0 ]; then
            pass "Branch is up to date with remote."
            return 0
        fi
        echo "  $ahead commit(s) ahead of remote."
    else
        echo "  No upstream tracking branch set."
    fi

    confirm "Push $BRANCH to origin?"
    git push -u origin "$BRANCH"
    pass "Pushed."
}

# -----------------------------------------------------------------------
# Step 5: Open PR
# -----------------------------------------------------------------------

do_open_pr() {
    if $ON_MASTER; then return 0; fi

    step_header "Open PR to master"

    if ! command -v gh &>/dev/null; then
        fail "gh CLI not found. Install from https://cli.github.com/"
    fi

    # Check for existing PR
    local existing
    existing=$(gh pr list --head "$BRANCH" --base master --state open --json number --jq '.[0].number' 2>/dev/null || true)
    if [ -n "$existing" ]; then
        echo "  PR #$existing already exists."
        PR_NUM="$existing"
        gh pr view "$PR_NUM" --json title,url --jq '"  " + .title + "\n  " + .url'
        return 0
    fi

    echo "  No open PR found for $BRANCH -> master."
    confirm "Create PR?"

    local pr_url
    pr_url=$(gh pr create \
        --base master \
        --head "$BRANCH" \
        --title "Release $VER_STRING" \
        --body "Release $VER_STRING. See CHANGELOG.md for details.")

    PR_NUM=$(gh pr view "$pr_url" --json number --jq '.number')
    echo "  Created: $pr_url"
}

# -----------------------------------------------------------------------
# Step 6: Wait for CI
# -----------------------------------------------------------------------

do_wait_ci() {
    if $ON_MASTER; then return 0; fi

    step_header "Wait for CI on PR #$PR_NUM"
    echo "  Polling every 30s (Ctrl-C to abort)..."
    echo ""

    while true; do
        local checks_json status_summary
        checks_json=$(gh pr checks "$PR_NUM" --json name,state 2>/dev/null || true)

        if [ -z "$checks_json" ] || [ "$checks_json" = "[]" ]; then
            echo "  Waiting for checks to start..."
            sleep 30
            continue
        fi

        # Show current state
        echo "$checks_json" | python3 -c "
import sys, json
checks = json.load(sys.stdin)
for c in checks:
    icon = {'SUCCESS':'ok','FAILURE':'FAIL','PENDING':'...'}.get(c['state'], c['state'])
    print(f\"  [{icon:>4}] {c['name']}\")
" 2>/dev/null || echo "$checks_json"

        local any_pending any_failed
        any_pending=$(echo "$checks_json" | python3 -c "
import sys, json
checks = json.load(sys.stdin)
print('yes' if any(c['state'] == 'PENDING' for c in checks) else 'no')
" 2>/dev/null || echo "no")

        any_failed=$(echo "$checks_json" | python3 -c "
import sys, json
checks = json.load(sys.stdin)
print('yes' if any(c['state'] == 'FAILURE' for c in checks) else 'no')
" 2>/dev/null || echo "no")

        if [ "$any_failed" = "yes" ]; then
            echo ""
            fail "One or more CI checks failed. Fix the issue and re-run."
        fi

        if [ "$any_pending" = "no" ]; then
            echo ""
            pass "All CI checks passed."
            return 0
        fi

        echo "  ... waiting 30s"
        echo ""
        sleep 30
    done
}

# -----------------------------------------------------------------------
# Step 7: Merge PR
# -----------------------------------------------------------------------

do_merge_pr() {
    if $ON_MASTER; then return 0; fi

    step_header "Merge PR #$PR_NUM to master"

    confirm "Merge PR #$PR_NUM (squash)?"
    gh pr merge "$PR_NUM" --squash --delete-branch
    pass "Merged and branch deleted on remote."
}

# -----------------------------------------------------------------------
# Step 8: Switch to master
# -----------------------------------------------------------------------

do_switch_master() {
    if $ON_MASTER; then
        echo ""
        echo "  (Already on master, pulling latest)"
        git pull --ff-only origin master
        return 0
    fi

    step_header "Switch to master and pull"

    git checkout master
    git pull --ff-only origin master
    BRANCH="master"
    ON_MASTER=true
    pass "On master at $(git rev-parse --short HEAD)."
}

# -----------------------------------------------------------------------
# Step 9: Verify on master
# -----------------------------------------------------------------------

do_verify_master() {
    step_header "Verify build on master"

    make clean >/dev/null 2>&1
    make tests 2>&1
    pass "All tests pass on master."
}

# -----------------------------------------------------------------------
# Step 10: Tag and push
# -----------------------------------------------------------------------

do_tag() {
    step_header "Create and push tag $VER_TAG"

    confirm "Create annotated tag $VER_TAG and push to origin?"
    git tag -a "$VER_TAG" -m "Release $VER_STRING"
    git push origin "$VER_TAG"
    pass "Tag $VER_TAG pushed. Release workflow triggered."
}

# -----------------------------------------------------------------------
# Step 11: Wait for release
# -----------------------------------------------------------------------

do_wait_release() {
    if [ "$MODE" = "release-local" ]; then
        step_header "Create GitHub release (local)"
        gh release create "$VER_TAG" \
            --title "xelp $VER_STRING" \
            --notes "Release $VER_STRING. See CHANGELOG.md for details."
        pass "Release created locally."
        return 0
    fi

    step_header "Wait for GitHub Release (created by release.yml)"
    echo "  Polling every 30s (Ctrl-C to abort)..."

    local attempts=0
    while [ $attempts -lt 40 ]; do
        local release_url
        release_url=$(gh release view "$VER_TAG" --json url --jq '.url' 2>/dev/null || true)
        if [ -n "$release_url" ]; then
            echo ""
            pass "Release published!"
            echo "  $release_url"
            return 0
        fi
        attempts=$((attempts + 1))
        echo "  ... not yet (attempt $attempts/40)"
        sleep 30
    done

    echo ""
    echo "  Timed out waiting for release workflow."
    echo "  Check: https://github.com/deftio/xelp/actions"
    echo "  You can create the release manually:"
    echo "    gh release create $VER_TAG --title 'xelp $VER_STRING' --notes 'See CHANGELOG.md'"
    exit 1
}

# -----------------------------------------------------------------------
# Step 12: Done
# -----------------------------------------------------------------------

do_done() {
    echo ""
    echo "============================================"
    echo "  xelp $VER_STRING released successfully."
    echo "  Tag: $VER_TAG"
    local release_url
    release_url=$(gh release view "$VER_TAG" --json url --jq '.url' 2>/dev/null || true)
    if [ -n "$release_url" ]; then
        echo "  URL: $release_url"
    fi
    echo "============================================"
}

# -----------------------------------------------------------------------
# Main
# -----------------------------------------------------------------------

PR_NUM=""

case "${1:-}" in
    --validate)
        MODE="validate" ;;
    --release-local)
        MODE="release-local" ;;
    --help|-h)
        echo "Usage: bash tools/make_release.sh [--validate|--release-local]"
        echo ""
        echo "  (no args)        Full guided release (recommended)"
        echo "  --validate       Local validation only (build, tests, coverage)"
        echo "  --release-local  Full flow but create GH release locally (fallback)"
        echo ""
        echo "See tools/make-release.md for documentation."
        exit 0
        ;;
    "")
        MODE="full" ;;
    *)
        echo "Unknown option: $1"
        echo "Run with --help for usage."
        exit 1
        ;;
esac

echo "============================================"
echo "  xelp release pipeline"
echo "  Mode: $MODE"
echo "============================================"

# -- Always run --
do_extract_version
do_validate

if [ "$MODE" = "validate" ]; then
    echo ""
    echo "============================================"
    echo "  Validation passed."
    echo "  Version: $VER_STRING ($VER_HEX)"
    echo "============================================"
    exit 0
fi

# -- Full release flow --
do_check_git
do_push_branch
do_open_pr
do_wait_ci
do_merge_pr
do_switch_master
do_verify_master
do_tag
do_wait_release
do_done
