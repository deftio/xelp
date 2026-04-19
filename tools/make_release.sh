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

# Log all output to build/release-<timestamp>.log
mkdir -p build
LOG_FILE="build/release-$(date -u '+%Y%m%d-%H%M%S').log"
exec > >(tee -a "$LOG_FILE") 2>&1
echo "Log: $LOG_FILE"

STEP=0
MODE="full"
BRANCH=""
ON_MASTER=false
TAG_EXISTS=false

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

# Print a command before running it so the user sees exactly what executes.
# Usage: run_cmd make clean
#        run_cmd git push -u origin "$BRANCH"
run_cmd() {
    echo "  \$ $*"
    "$@"
}

# -----------------------------------------------------------------------
# Step 1: Extract version
# -----------------------------------------------------------------------

do_extract_version() {
    step_header "Extract version from xelp.h"

    mkdir -p build
    run_cmd gcc tools/extract_version.c -Isrc -o build/extract_version \
        || fail "Could not compile extract_version.c"

    run_cmd build/extract_version build/xelp_version.yaml \
        || fail "extract_version failed"

    VER_HEX=$(sed -n 's/^version_hex: "\(.*\)"/\1/p' build/xelp_version.yaml)
    VER_STRING=$(sed -n 's/^version: "\(.*\)"/\1/p' build/xelp_version.yaml)
    VER_TAG=$(sed -n 's/^tag: "\(.*\)"/\1/p' build/xelp_version.yaml)

    [ -n "$VER_STRING" ] || fail "No version string produced"

    echo "  Version: $VER_STRING ($VER_HEX)"
    echo "  Tag:     $VER_TAG"
    cat build/xelp_version.yaml
}

# -----------------------------------------------------------------------
# Step 1b: Sync version in library manifests
# -----------------------------------------------------------------------

do_sync_manifests() {
    step_header "Sync version in library.json, library.properties, idf_component.yml"

    # Strip leading 'v' if present (VER_STRING is e.g. "0.2.5")
    local ver="$VER_STRING"

    # library.json
    if [ -f library.json ]; then
        local cur_lj
        cur_lj=$(python3 -c "import json; print(json.load(open('library.json'))['version'])" 2>/dev/null || true)
        if [ "$cur_lj" = "$ver" ]; then
            pass "library.json already at $ver"
        else
            echo "  library.json: $cur_lj -> $ver"
            python3 -c "
import json, pathlib
p = pathlib.Path('library.json')
d = json.loads(p.read_text())
d['version'] = '$ver'
p.write_text(json.dumps(d, indent=4) + '\n')
"
            git add library.json
            pass "library.json updated to $ver"
        fi
    fi

    # library.properties
    if [ -f library.properties ]; then
        local cur_lp
        cur_lp=$(sed -n 's/^version=//p' library.properties)
        if [ "$cur_lp" = "$ver" ]; then
            pass "library.properties already at $ver"
        else
            echo "  library.properties: $cur_lp -> $ver"
            sed -i.bak "s/^version=.*/version=$ver/" library.properties
            rm -f library.properties.bak
            git add library.properties
            pass "library.properties updated to $ver"
        fi
    fi

    # idf_component.yml (ESP-IDF Component Registry)
    if [ -f idf_component.yml ]; then
        local cur_idf
        cur_idf=$(sed -n 's/^version: "\(.*\)"/\1/p' idf_component.yml)
        if [ "$cur_idf" = "$ver" ]; then
            pass "idf_component.yml already at $ver"
        else
            echo "  idf_component.yml: $cur_idf -> $ver"
            sed -i.bak "s/^version: \".*\"/version: \"$ver\"/" idf_component.yml
            rm -f idf_component.yml.bak
            git add idf_component.yml
            pass "idf_component.yml updated to $ver"
        fi
    fi
}

# -----------------------------------------------------------------------
# Step 1c: Update version badges
# -----------------------------------------------------------------------

do_update_badges() {
    echo "==> Updating version badges..."
    python3 tools/update_badges.py
    git add README.md pages/index.html
}

# -----------------------------------------------------------------------
# Step 2: Local validation
# -----------------------------------------------------------------------

do_validate() {
    step_header "Local validation (build, tests, warnings, coverage)"

    echo "  --- Clean build + tests ---"
    run_cmd make clean >/dev/null 2>&1
    run_cmd make tests 2>&1
    pass "All tests passed."

    echo ""
    echo "  --- Zero-warning check ---"
    local build_log warnings
    echo "  \$ make clean && make tests  (capturing output for warning scan)"
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
    echo "  \$ gcov src/xelp.c"
    cov_line=$(gcov src/xelp.c 2>/dev/null | grep "Lines executed" | head -1)
    pct=$(echo "$cov_line" | grep -o '[0-9]*\.[0-9]*%' | head -1)
    pass "Coverage: $pct"
}

# -----------------------------------------------------------------------
# Step 3: Check git state
# -----------------------------------------------------------------------

do_check_git() {
    step_header "Check git state"

    echo "  \$ git rev-parse --abbrev-ref HEAD"
    BRANCH=$(git rev-parse --abbrev-ref HEAD)
    echo "  Branch: $BRANCH"

    if [ "$BRANCH" = "master" ] || [ "$BRANCH" = "main" ]; then
        ON_MASTER=true
        echo "  Already on $BRANCH -- will skip PR steps."
    fi

    # Tag check
    echo "  \$ git tag -l $VER_TAG"
    if git tag -l "$VER_TAG" | grep -q "$VER_TAG"; then
        echo "  Tag $VER_TAG already exists."
        if [ "$MODE" != "validate" ]; then
            # Already tagged -- check if release exists too
            local existing_release
            existing_release=$(gh release view "$VER_TAG" --json url --jq '.url' 2>/dev/null || true)
            if [ -n "$existing_release" ]; then
                echo ""
                pass "Release already published: $existing_release"
                echo "  Nothing to do. Bump XELP_VERSION for the next release."
                exit 0
            fi
            echo "  Tag exists but no GitHub Release yet -- will skip to release step."
            TAG_EXISTS=true
        else
            fail "Tag $VER_TAG already exists. Bump XELP_VERSION in src/xelp.h first."
        fi
    else
        pass "Tag $VER_TAG does not exist yet."
    fi

    # Working tree
    local status
    echo "  \$ git status --porcelain"
    status=$(git status --porcelain)
    if [ -n "$status" ]; then
        echo ""
        echo "  Uncommitted changes:"
        run_cmd git status --short
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
# Step 4: Sync with master and push branch
# -----------------------------------------------------------------------

do_push_branch() {
    if $ON_MASTER; then return 0; fi

    step_header "Sync with master and push branch '$BRANCH'"

    # Merge master into branch to avoid conflicts and ensure CI runs
    run_cmd git fetch origin master
    local behind
    behind=$(git rev-list --count "HEAD..origin/master" 2>/dev/null || echo "0")
    if [ "$behind" -gt 0 ]; then
        echo "  Branch is $behind commit(s) behind origin/master."
        echo "  Merging origin/master (preferring branch on conflicts)..."
        if ! run_cmd git merge origin/master -X ours --no-edit; then
            # Handle modify/delete conflicts: keep our versions
            local unresolved
            unresolved=$(git diff --name-only --diff-filter=U 2>/dev/null || true)
            if [ -n "$unresolved" ]; then
                echo "  Resolving remaining conflicts (keeping branch versions)..."
                echo "$unresolved" | while read -r f; do
                    git checkout --ours "$f" 2>/dev/null && git add "$f" || git add "$f"
                done
                run_cmd git commit --no-edit
            fi
        fi
        pass "Merged origin/master into $BRANCH."
    else
        pass "Branch is up to date with master."
    fi

    # Push
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
    run_cmd git push -u origin "$BRANCH"
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
    echo "  \$ gh pr list --head $BRANCH --base master --state open"
    existing=$(gh pr list --head "$BRANCH" --base master --state open --json number --jq '.[0].number' 2>/dev/null || true)
    if [ -n "$existing" ]; then
        echo "  PR #$existing already exists."
        PR_NUM="$existing"
        run_cmd gh pr view "$PR_NUM" --json title,url --jq '"  " + .title + "\n  " + .url'
        return 0
    fi

    echo "  No open PR found for $BRANCH -> master."
    confirm "Create PR?"

    echo "  \$ gh pr create --base master --head $BRANCH --title \"Release $VER_STRING\""
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
    echo "  Polling every 30s..."
    echo ""

    local attempts=0
    local max_attempts=40
    while [ $attempts -lt $max_attempts ]; do
        local checks_json status_summary
        echo "  \$ gh pr checks $PR_NUM"
        checks_json=$(gh pr checks "$PR_NUM" --json name,state 2>/dev/null || true)

        if [ -z "$checks_json" ] || [ "$checks_json" = "[]" ]; then
            attempts=$((attempts + 1))
            echo "  Waiting for checks to start... (attempt $attempts/$max_attempts)"
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

        attempts=$((attempts + 1))
        echo "  ... waiting 30s (attempt $attempts/$max_attempts)"
        echo ""
        sleep 30
    done

    echo ""
    echo "  Timed out waiting for CI checks after $max_attempts attempts."
    echo "  CI may not be configured to run on this branch."
    confirm "Continue without CI? (not recommended)"
}

# -----------------------------------------------------------------------
# Step 7: Merge PR
# -----------------------------------------------------------------------

do_merge_pr() {
    if $ON_MASTER; then return 0; fi

    step_header "Enable auto-merge on PR #$PR_NUM"

    # Check if PR is already merged (re-run scenario)
    local pr_state
    pr_state=$(gh pr view "$PR_NUM" --json state --jq '.state' 2>/dev/null || true)
    if [ "$pr_state" = "MERGED" ]; then
        pass "PR #$PR_NUM is already merged."
        return 0
    fi

    confirm "Enable auto-merge (squash) for PR #$PR_NUM?"
    run_cmd gh pr merge "$PR_NUM" --auto --squash --delete-branch
    pass "Auto-merge enabled. Will merge when all requirements are met."
}

# -----------------------------------------------------------------------
# Step 8: Wait for merge, switch to master
# -----------------------------------------------------------------------

do_switch_master() {
    if $ON_MASTER; then
        echo ""
        echo "  (Already on master, pulling latest)"
        run_cmd git pull --ff-only origin master
        return 0
    fi

    step_header "Wait for PR merge, then switch to master"
    echo "  Polling every 15s for merge completion..."

    local attempts=0
    while [ $attempts -lt 80 ]; do
        local pr_state
        echo "  \$ gh pr view $PR_NUM --json state"
        pr_state=$(gh pr view "$PR_NUM" --json state --jq '.state' 2>/dev/null || true)
        if [ "$pr_state" = "MERGED" ]; then
            echo ""
            pass "PR #$PR_NUM merged."
            break
        elif [ "$pr_state" = "CLOSED" ]; then
            fail "PR #$PR_NUM was closed without merging."
        fi
        attempts=$((attempts + 1))
        echo "  ... PR state: ${pr_state:-unknown} (attempt $attempts/80)"
        sleep 15
    done

    if [ $attempts -ge 80 ]; then
        fail "Timed out waiting for PR to merge. Check branch protection requirements."
    fi

    run_cmd git checkout master
    run_cmd git pull --ff-only origin master
    BRANCH="master"
    ON_MASTER=true
    pass "On master at $(git rev-parse --short HEAD)."
}

# -----------------------------------------------------------------------
# Step 9: Verify on master
# -----------------------------------------------------------------------

do_verify_master() {
    step_header "Verify build on master"

    run_cmd make clean >/dev/null 2>&1
    run_cmd make tests 2>&1
    pass "All tests pass on master."
}

# -----------------------------------------------------------------------
# Step 10: Tag and push
# -----------------------------------------------------------------------

do_tag() {
    step_header "Create and push tag $VER_TAG"

    confirm "Create annotated tag $VER_TAG and push to origin?"
    run_cmd git tag -a "$VER_TAG" -m "Release $VER_STRING"
    run_cmd git push origin "$VER_TAG"
    pass "Tag $VER_TAG pushed. Release workflow triggered."
}

# -----------------------------------------------------------------------
# Step 11: Wait for release
# -----------------------------------------------------------------------

do_wait_release() {
    if [ "$MODE" = "release-local" ]; then
        step_header "Create GitHub release (local)"
        run_cmd gh release create "$VER_TAG" \
            --title "xelp $VER_STRING" \
            --notes "Release $VER_STRING. See CHANGELOG.md for details."
        pass "Release created locally."
        return 0
    fi

    step_header "Wait for GitHub Release (created by release.yml)"
    echo "  Polling every 30s..."

    local attempts=0
    while [ $attempts -lt 40 ]; do
        # Check if release exists
        local release_url
        echo "  \$ gh release view $VER_TAG"
        release_url=$(gh release view "$VER_TAG" --json url --jq '.url' 2>/dev/null || true)
        if [ -n "$release_url" ]; then
            echo ""
            pass "Release published!"
            echo "  $release_url"
            return 0
        fi

        # Check if Release workflow failed
        local run_conclusion
        run_conclusion=$(gh api repos/:owner/:repo/actions/runs \
            --jq ".workflow_runs[] | select(.name==\"Release\" and .head_branch==\"$VER_TAG\") | .conclusion" \
            2>/dev/null | head -1 || true)
        if [ "$run_conclusion" = "failure" ]; then
            echo ""
            echo "  Release workflow FAILED. Fetching error log..."
            local run_id
            run_id=$(gh api repos/:owner/:repo/actions/runs \
                --jq ".workflow_runs[] | select(.name==\"Release\" and .head_branch==\"$VER_TAG\") | .id" \
                2>/dev/null | head -1 || true)
            if [ -n "$run_id" ]; then
                gh run view "$run_id" --log-failed 2>&1 | tail -20
                echo ""
                echo "  Full log: gh run view $run_id --log-failed"
            fi
            echo ""
            echo "  Fix the issue, delete the tag, and re-run:"
            echo "    git tag -d $VER_TAG && git push origin :refs/tags/$VER_TAG"
            echo "  Or create the release manually:"
            echo "    gh release create $VER_TAG --title 'xelp $VER_STRING' --notes 'See CHANGELOG.md'"
            exit 1
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
# Step 12: Publish to PlatformIO registry
# -----------------------------------------------------------------------

do_pio_publish() {
    step_header "Publish to PlatformIO registry"

    if ! command -v pio &>/dev/null; then
        fail "pio CLI not found. Install: pip install platformio"
    fi

    # Check if already published at this version
    local pio_info
    pio_info=$(pio pkg show deftio/xelp 2>/dev/null || true)
    if echo "$pio_info" | grep -q "$VER_STRING"; then
        pass "xelp $VER_STRING already on PlatformIO registry."
        return 0
    fi

    confirm "Publish xelp $VER_STRING to PlatformIO?"
    run_cmd pio pkg publish . --no-interactive
    pass "Published to PlatformIO registry."
}

# -----------------------------------------------------------------------
# Step 13: Publish to ESP-IDF Component Registry
# -----------------------------------------------------------------------

do_idf_publish() {
    step_header "Publish to ESP-IDF Component Registry"

    if ! command -v compote &>/dev/null; then
        fail "compote CLI not found. Install: pip install idf-component-manager"
    fi

    confirm "Publish xelp $VER_STRING to ESP-IDF Component Registry?"
    echo "  --- Packing component ---"
    run_cmd compote component pack --name xelp
    echo "  --- Uploading component ---"
    run_cmd compote component upload --name xelp
    pass "Published to ESP-IDF Component Registry."
}

# -----------------------------------------------------------------------
# Step 14: Done
# -----------------------------------------------------------------------

do_done() {
    echo ""
    echo "============================================"
    echo "  xelp $VER_STRING released successfully."
    echo "  Tag: $VER_TAG"
    local release_url
    release_url=$(gh release view "$VER_TAG" --json url --jq '.url' 2>/dev/null || true)
    if [ -n "$release_url" ]; then
        echo "  GitHub: $release_url"
    fi
    echo "============================================"
    echo ""
    echo "  Reminder: Arduino Library Manager indexes from GitHub tags."
    echo "  If xelp is registered, the new version will appear automatically."
    echo "  To register: submit a PR to https://github.com/arduino/library-registry"
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
do_sync_manifests
do_update_badges
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

if $TAG_EXISTS; then
    # Re-run: tag exists but release doesn't -- just wait for it
    do_wait_release
    do_pio_publish
    do_idf_publish
    do_done
else
    do_push_branch
    do_open_pr
    do_wait_ci
    do_merge_pr
    do_switch_master
    do_verify_master
    do_tag
    do_wait_release
    do_pio_publish
    do_idf_publish
    do_done
fi
