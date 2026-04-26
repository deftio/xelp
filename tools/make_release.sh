#!/usr/bin/env bash
#
# make_release.sh -- Guided release pipeline for xelp.
#
# Walks through every step from local validation to published GitHub
# Release and package registry uploads, pausing for confirmation before
# anything visible to others.
#
# The XELP_VERSION macro in src/xelp.h is the single source of truth;
# the version is read via the C preprocessor (tools/extract_version.c),
# not regex.
#
# Usage:
#   bash tools/make_release.sh                 # full guided release
#   bash tools/make_release.sh --validate      # local validation only
#   bash tools/make_release.sh --release-local # full flow, local GH release fallback
#
# Exit status: 0 if every step passes, non-zero on first failure.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
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
    echo "" >&2
    echo "  FAIL: $1" >&2
    echo "" >&2
    echo "  The release pipeline stopped at step $STEP." >&2
    echo "  Fix the issue above and re-run: bash tools/make_release.sh" >&2
    echo "  Log: $LOG_FILE" >&2
    exit 1
}

pass() {
    echo "  PASS: $1"
}

# Print a command before running it so the user sees exactly what executes.
run_cmd() {
    echo "  \$ $*"
    "$@"
}

# Run make clean while preserving build/xelp_version.yaml (needed by later steps).
safe_clean() {
    local saved=""
    if [ -f build/xelp_version.yaml ]; then
        saved=$(cat build/xelp_version.yaml)
    fi
    make clean >/dev/null 2>&1 || true
    if [ -n "$saved" ]; then
        mkdir -p build
        echo "$saved" > build/xelp_version.yaml
    fi
}

# -----------------------------------------------------------------------
# Step 1: Extract version from xelp.h
# -----------------------------------------------------------------------

do_extract_version() {
    step_header "Extract version from xelp.h"

    mkdir -p build
    run_cmd gcc tools/extract_version.c -Isrc -o build/extract_version \
        || fail "Could not compile extract_version.c.
  Make sure gcc is installed and src/xelp.h exists."

    run_cmd build/extract_version build/xelp_version.yaml \
        || fail "extract_version failed to write build/xelp_version.yaml."

    VER_HEX=$(sed -n 's/^version_hex: "\(.*\)"/\1/p' build/xelp_version.yaml)
    VER_STRING=$(sed -n 's/^version: "\(.*\)"/\1/p' build/xelp_version.yaml)
    VER_TAG=$(sed -n 's/^tag: "\(.*\)"/\1/p' build/xelp_version.yaml)

    [ -n "$VER_STRING" ] || fail "No version string in build/xelp_version.yaml.
  Check that XELP_VERSION is defined in src/xelp.h."

    # Enforce three-component semver (e.g. 0.3.0, never 0.3)
    if ! echo "$VER_STRING" | grep -qE '^[0-9]+\.[0-9]+\.[0-9]+$'; then
        fail "Version '$VER_STRING' is not three-component semver (expected X.Y.Z).
  Fix XELP_VERSION in src/xelp.h to use format 0x00MMmmpp."
    fi

    echo "  Version: $VER_STRING ($VER_HEX)"
    echo "  Tag:     $VER_TAG"
    cat build/xelp_version.yaml
}

# -----------------------------------------------------------------------
# Step 2: Sync version in library manifests
# -----------------------------------------------------------------------

do_sync_manifests() {
    step_header "Sync version in library.json, library.properties, idf_component.yml"

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
            pass "idf_component.yml updated to $ver"
        fi
    fi
}

# -----------------------------------------------------------------------
# Step 3: Update version badges
# -----------------------------------------------------------------------

do_update_badges() {
    step_header "Update version badges in README.md and pages/index.html"

    if [ ! -f build/xelp_version.yaml ]; then
        fail "build/xelp_version.yaml not found. Run extract_version first."
    fi
    run_cmd python3 tools/update_badges.py
    pass "Badges updated."
}

# -----------------------------------------------------------------------
# Step 4: Local validation (tests, examples, warnings, coverage)
# -----------------------------------------------------------------------

do_validate() {
    step_header "Local validation (tests, examples, warnings, coverage)"

    echo "  --- Clean build + tests + examples ---"
    local build_log warnings
    safe_clean
    echo "  \$ make validate  (tests + examples, capturing for warning scan)"
    build_log=$(make validate 2>&1) || {
        echo "$build_log"
        fail "make validate failed.
  Fix the build/test errors above and re-run."
    }
    echo "$build_log" | tail -5
    pass "Tests passed, all examples built."

    echo ""
    echo "  --- Zero-warning check ---"
    warnings=$(echo "$build_log" | grep -E "^[^:]+\.(c|h):[0-9]+:[0-9]+: warning:" || true)
    if [ -n "$warnings" ]; then
        echo "$warnings" >&2
        fail "Compiler warnings detected.
  Fix the warnings above (xelp requires zero warnings)."
    fi
    pass "Zero warnings."

    echo ""
    echo "  --- Coverage ---"
    local cov_line pct
    echo "  \$ gcov -o build/ src/xelp.c"
    cov_line=$(gcov -o build/ src/xelp.c 2>/dev/null | grep "Lines executed" | head -1)
    pct=$(echo "$cov_line" | grep -o '[0-9]*\.[0-9]*%' | head -1)
    pass "Coverage: $pct"

    echo ""
    echo "  --- Cleaning build artifacts ---"
    safe_clean
    pass "Build artifacts cleaned."
}

# -----------------------------------------------------------------------
# Step 5: Cross-compile all targets (Docker) and update size tables
# -----------------------------------------------------------------------

do_crossbuild() {
    step_header "Cross-compile all targets (Docker)"

    if ! command -v docker &>/dev/null; then
        echo "  Docker not found -- skipping cross-compilation."
        echo "  Size tables will use existing data."
        return 0
    fi

    echo "  Running Docker cross-build (this takes a few minutes)..."
    run_cmd bash tools/crossbuild.sh
    pass "Cross-compilation complete."

    if [ -f build/sizes.csv ]; then
        echo ""
        echo "  --- Updating size tables in README.md and pages/index.html ---"
        run_cmd bash tools/update_sizes.sh
        pass "Size tables updated from build/sizes.csv."
    else
        echo "  WARNING: build/sizes.csv not produced. Size tables unchanged."
    fi
}

# -----------------------------------------------------------------------
# Step 6: Commit pipeline-generated changes
# -----------------------------------------------------------------------

# Files the pipeline itself may modify (version sync, badge update,
# size tables, crossbuild). Anything outside this list is unexpected
# and should block the release.
PIPELINE_FILES="README.md pages/index.html library.json library.properties idf_component.yml"

do_commit_pipeline_changes() {
    step_header "Commit pipeline-generated changes"

    local status
    status=$(git status --porcelain)
    if [ -z "$status" ]; then
        pass "Working tree is clean -- nothing to commit."
        return 0
    fi

    # Split dirty files into expected (pipeline) and unexpected.
    local unexpected=""
    local to_commit=""
    while IFS= read -r line; do
        # git status --porcelain: first two chars are status, then space, then path
        local file="${line:3}"
        local found=false
        for known in $PIPELINE_FILES; do
            if [ "$file" = "$known" ]; then
                found=true
                break
            fi
        done
        if $found; then
            to_commit="$to_commit $file"
        else
            unexpected="$unexpected $file"
        fi
    done <<< "$status"

    if [ -n "$unexpected" ]; then
        echo ""
        echo "  Unexpected uncommitted files:"
        for f in $unexpected; do
            echo "    $f"
        done
        echo ""
        fail "Commit or stash these files before running the release pipeline.
  Only pipeline-generated files ($PIPELINE_FILES) are auto-committed."
    fi

    echo "  Modified by pipeline:"
    for f in $to_commit; do
        echo "    $f"
    done

    confirm "Commit these files?"
    # shellcheck disable=SC2086
    git add $to_commit
    git commit -m "sync manifests, badges, and sizes for $VER_STRING"
    pass "Committed pipeline-generated changes."
}

# -----------------------------------------------------------------------
# Step 7: Check git state
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
            fail "Tag $VER_TAG already exists.
  Bump XELP_VERSION in src/xelp.h first."
        fi
    else
        pass "Tag $VER_TAG does not exist yet."
    fi

    # Working tree -- should be clean after do_commit_pipeline_changes
    local status
    echo "  \$ git status --porcelain"
    status=$(git status --porcelain)
    if [ -n "$status" ]; then
        echo ""
        echo "  Uncommitted changes:"
        run_cmd git status --short
        fail "Working tree is dirty. Commit or stash before releasing.
  If these are pipeline changes, re-run and the script will auto-commit them."
    else
        pass "Working tree is clean."
    fi
}

# -----------------------------------------------------------------------
# Step 8: Sync with master and push branch
# -----------------------------------------------------------------------

do_push_branch() {
    step_header "Push $BRANCH to origin"

    run_cmd git fetch origin master

    if ! $ON_MASTER; then
        # Feature branch: only merge master if it has commits we don't have.
        # If master is already an ancestor of HEAD, the branch already contains
        # everything on master and no merge is needed. This is the normal case
        # for a single-maintainer project.
        if git merge-base --is-ancestor origin/master HEAD; then
            pass "Branch already contains all of origin/master -- no merge needed."
        else
            local behind
            behind=$(git rev-list --count "HEAD..origin/master" 2>/dev/null || echo "0")
            echo "  Branch is $behind commit(s) behind origin/master."
            echo "  Merging origin/master..."
            if ! run_cmd git merge origin/master --no-edit; then
                fail "Merge conflict. Resolve manually and re-run.
  Commands to resolve:
    git status                       # see conflicted files
    # ... edit and fix conflicts ...
    git add <resolved-files>
    git commit
    bash tools/make_release.sh       # re-run"
            fi
            pass "Merged origin/master into $BRANCH."
        fi
    fi

    # Check if remote branch exists and whether we're ahead.
    local remote_exists=true
    if ! git rev-parse --verify "origin/$BRANCH" &>/dev/null; then
        remote_exists=false
    fi

    if $remote_exists; then
        local ahead
        ahead=$(git rev-list --count "origin/$BRANCH..HEAD" 2>/dev/null || echo "0")
        if [ "$ahead" -eq 0 ]; then
            pass "$BRANCH is up to date with origin."
            return 0
        fi
        echo "  $ahead commit(s) ahead of origin/$BRANCH."
    else
        echo "  Remote branch origin/$BRANCH does not exist yet."
    fi

    confirm "Push $BRANCH to origin?"
    run_cmd git push -u origin "$BRANCH"
    pass "Pushed."
}

# -----------------------------------------------------------------------
# Step 9: Open PR
# -----------------------------------------------------------------------

do_open_pr() {
    if $ON_MASTER; then return 0; fi

    step_header "Open PR to master"

    if ! command -v gh &>/dev/null; then
        fail "gh CLI not found.
  Install from https://cli.github.com/ and authenticate with: gh auth login"
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
# Step 10: Wait for CI
# -----------------------------------------------------------------------

do_wait_ci() {
    if $ON_MASTER; then
        step_header "Wait for CI on master push"
        echo "  Polling commit status every 30s..."
        echo ""
        local sha
        sha=$(git rev-parse HEAD)

        local attempts=0
        local max_attempts=40
        while [ $attempts -lt $max_attempts ]; do
            # Check combined commit status API
            local state
            echo "  \$ gh api repos/:owner/:repo/commits/$sha/status"
            state=$(gh api "repos/{owner}/{repo}/commits/$sha/status" --jq '.state' 2>/dev/null || echo "pending")
            echo "  Combined status: $state"

            if [ "$state" = "failure" ] || [ "$state" = "error" ]; then
                echo ""
                fail "CI failed on master.
  Check: https://github.com/deftio/xelp/actions
  Fix the issue and re-run."
            fi

            if [ "$state" = "success" ]; then
                echo ""
                pass "All CI checks passed on master."
                return 0
            fi

            # Also check via check-runs API (GitHub Actions uses this)
            local any_in_progress any_failed_cr
            any_in_progress=$(gh api "repos/{owner}/{repo}/commits/$sha/check-runs" \
                --jq '[.check_runs[] | select(.status != "completed")] | length' 2>/dev/null || echo "1")
            any_failed_cr=$(gh api "repos/{owner}/{repo}/commits/$sha/check-runs" \
                --jq '[.check_runs[] | select(.conclusion == "failure")] | length' 2>/dev/null || echo "0")

            if [ "$any_failed_cr" -gt 0 ]; then
                echo ""
                fail "CI check-run failed on master.
  Check: https://github.com/deftio/xelp/actions
  Fix the issue and re-run."
            fi
            if [ "$any_in_progress" -eq 0 ]; then
                echo ""
                pass "All CI check-runs passed on master."
                return 0
            fi

            attempts=$((attempts + 1))
            echo "  ... waiting 30s (attempt $attempts/$max_attempts)"
            echo ""
            sleep 30
        done

        echo ""
        echo "  Timed out waiting for CI after $max_attempts attempts."
        confirm "Continue without CI? (not recommended)"
        return 0
    fi

    step_header "Wait for CI on PR #$PR_NUM"
    echo "  Polling every 30s..."
    echo ""

    local attempts=0
    local max_attempts=40
    while [ $attempts -lt $max_attempts ]; do
        local checks_json
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
            fail "One or more CI checks failed.
  Check: https://github.com/deftio/xelp/actions
  Fix the issue and re-run."
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
# Step 11: Merge PR
# -----------------------------------------------------------------------

do_merge_pr() {
    if $ON_MASTER; then return 0; fi

    step_header "Squash-merge PR #$PR_NUM"

    # Check if PR is already merged (re-run scenario)
    local pr_state
    pr_state=$(gh pr view "$PR_NUM" --json state --jq '.state' 2>/dev/null || true)
    if [ "$pr_state" = "MERGED" ]; then
        pass "PR #$PR_NUM is already merged."
        return 0
    fi

    confirm "Squash-merge PR #$PR_NUM into master?"

    # Try direct merge first (CI already passed at this point).
    if gh pr merge "$PR_NUM" --squash --delete-branch 2>/dev/null; then
        pass "PR merged (squash)."
        return 0
    fi

    # Direct merge failed -- likely branch protection or auto-merge required.
    echo "  Direct merge blocked (branch protection?). Enabling auto-merge..."
    if gh pr merge "$PR_NUM" --auto --squash --delete-branch 2>/dev/null; then
        pass "Auto-merge enabled. Will merge when all requirements are met."
        return 0
    fi

    # Both failed -- give actionable guidance.
    fail "Could not merge PR #$PR_NUM.
  Check branch protection settings, required reviews, or status checks.
  You can merge manually:
    gh pr merge $PR_NUM --squash --delete-branch
  Then re-run this script to continue from where it stopped."
}

# -----------------------------------------------------------------------
# Step 12: Wait for merge, switch to master
# -----------------------------------------------------------------------

do_switch_master() {
    if $ON_MASTER; then
        step_header "Verify master is in sync with origin"
        run_cmd git fetch origin master
        local ahead behind
        ahead=$(git rev-list --count "origin/master..HEAD" 2>/dev/null || echo "0")
        behind=$(git rev-list --count "HEAD..origin/master" 2>/dev/null || echo "0")
        if [ "$ahead" -ne 0 ]; then
            fail "Local master is $ahead commit(s) ahead of origin.
  This should not happen. Push or reset before continuing."
        fi
        if [ "$behind" -ne 0 ]; then
            run_cmd git pull --ff-only origin master
        fi
        pass "master is in sync with origin at $(git rev-parse --short HEAD)."
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
            fail "PR #$PR_NUM was closed without merging.
  Re-open the PR or create a new one and re-run."
        fi
        attempts=$((attempts + 1))
        echo "  ... PR state: ${pr_state:-unknown} (attempt $attempts/80)"
        sleep 15
    done

    if [ $attempts -ge 80 ]; then
        fail "Timed out waiting for PR to merge.
  Check branch protection requirements and CI status."
    fi

    run_cmd git checkout master
    run_cmd git fetch origin master
    # After a squash-merge, local master and origin/master have diverged
    # (the squash commit is a new commit). Reset to origin/master which
    # has the authoritative squash-merged content.
    if ! git merge-base --is-ancestor origin/master HEAD 2>/dev/null; then
        echo "  Local master diverged from origin (expected after squash-merge)."
        run_cmd git reset --hard origin/master
    else
        run_cmd git pull --ff-only origin master
    fi
    BRANCH="master"
    ON_MASTER=true
    pass "On master at $(git rev-parse --short HEAD)."
}

# -----------------------------------------------------------------------
# Step 13: Verify on master
# -----------------------------------------------------------------------

do_verify_master() {
    step_header "Verify build on master"

    run_cmd make clean >/dev/null 2>&1
    echo "  \$ make validate"
    if ! make validate >/dev/null 2>&1; then
        fail "make validate failed on master.
  This should not happen after a successful PR merge."
    fi
    pass "All tests and examples pass on master."
}

# -----------------------------------------------------------------------
# Step 14: Tag and push
# -----------------------------------------------------------------------

do_tag() {
    step_header "Create and push tag $VER_TAG"

    confirm "Create annotated tag $VER_TAG and push to origin?"
    run_cmd git tag -a "$VER_TAG" -m "Release $VER_STRING"
    run_cmd git push origin "$VER_TAG"
    pass "Tag $VER_TAG pushed. Release workflow triggered."
}

# -----------------------------------------------------------------------
# Step 15: Wait for GitHub Release
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
            echo "  To retry: delete the tag and re-run:"
            echo "    git tag -d $VER_TAG && git push origin :refs/tags/$VER_TAG"
            echo "    bash tools/make_release.sh"
            echo ""
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
# Step 16: Publish to PlatformIO registry
# -----------------------------------------------------------------------

do_pio_publish() {
    step_header "Publish to PlatformIO registry"

    if ! command -v pio &>/dev/null; then
        echo "  pio CLI not found. Skipping."
        echo "  Install: pip install platformio"
        return 0
    fi

    # Check if already published at this version
    local pio_info
    pio_info=$(pio pkg show deftio/xelp 2>/dev/null || true)
    if echo "$pio_info" | grep -q "$VER_STRING"; then
        pass "xelp $VER_STRING already on PlatformIO registry."
        return 0
    fi

    confirm "Publish xelp $VER_STRING to PlatformIO?"
    run_cmd make clean >/dev/null 2>&1
    run_cmd pio pkg publish . --no-interactive
    pass "Published to PlatformIO registry."
}

# -----------------------------------------------------------------------
# Step 17: Publish to ESP-IDF Component Registry
# -----------------------------------------------------------------------

do_idf_publish() {
    step_header "Publish to ESP-IDF Component Registry"

    if ! command -v compote &>/dev/null; then
        echo "  compote CLI not found. Skipping."
        echo "  Install: pip install idf-component-manager"
        return 0
    fi

    confirm "Publish xelp $VER_STRING to ESP-IDF Component Registry?"
    echo "  --- Packing component ---"
    run_cmd compote component pack --name xelp
    echo "  --- Uploading component ---"
    run_cmd compote component upload --name xelp
    pass "Published to ESP-IDF Component Registry."
}

# -----------------------------------------------------------------------
# Step 18: Done
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
VER_STRING=""
VER_TAG=""
VER_HEX=""

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

# -- Always run (validation + pipeline prep) --
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
do_crossbuild
do_commit_pipeline_changes
do_check_git
do_sync_manifests
do_update_badges

# Auto-commit manifest and badge updates (if any files were staged)
if [ -n "$(git diff --cached --name-only)" ]; then
    step_header "Commit manifest and badge updates"
    run_cmd git commit -m "Sync manifests and badges for $VER_STRING"
    pass "Committed version sync changes."
fi

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
