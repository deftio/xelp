# Release Process

How to ship a release of xelp. The script `tools/make_release.sh` automates
every step below and pauses between each one so you can watch.

## Prerequisites

- `gcc` (build + tests)
- `gcov` (coverage)
- `gh` CLI, authenticated (`gh auth status`)
- Clean working tree recommended (script will prompt if dirty)

## Quick version

```bash
# Full guided release -- walks you through every step
bash tools/make_release.sh

# Just validate locally (no git, no PR, no tag)
bash tools/make_release.sh --validate
```

## What the script does

| Step | What happens | Needs confirmation? |
|------|-------------|---------------------|
| 1. Extract version | Compiles `extract_version.c`, reads `XELP_VERSION` from `xelp.h` via the C preprocessor, writes `build/xelp_version.yaml` | No |
| 2. Local validation | Clean build, runs all tests, checks zero warnings, checks coverage | No |
| 3. Git status | Shows uncommitted changes. If dirty, prompts you to commit or abort. | Yes, if dirty |
| 4. Push branch | Pushes current branch to origin | Yes |
| 5. Open PR | Creates a PR to `master` (or shows existing PR) | Yes |
| 6. Wait for CI | Polls `gh pr checks` until all checks pass or fail | Auto (Ctrl-C to abort) |
| 7. Merge PR | Merges the PR to master (squash merge) | Yes |
| 8. Switch to master | Checks out master and pulls | No |
| 9. Verify on master | Clean build + tests on the merged master | No |
| 10. Tag + push | Creates annotated tag `vX.Y.Z`, pushes to origin | Yes |
| 11. Wait for release | Polls until GitHub Release appears (created by `release.yml`) | Auto (Ctrl-C to abort) |
| 12. Done | Prints release URL | No |

If you are already on `master` with a clean tree (e.g. you merged the PR
manually), the script detects this and skips steps 4-8.

## Before you start

1. Bump `XELP_VERSION` in `src/xelp.h` to the new version
2. Update `CHANGELOG.md` -- move `[Unreleased]` items to a versioned heading
3. Commit those changes on your working branch

Then run:

```bash
bash tools/make_release.sh
```

## What if something fails?

| Problem | What to do |
|---------|------------|
| Build or tests fail | Fix the issue, commit, re-run the script |
| CI fails on the PR | Check the CI logs (`gh pr checks`), fix, push, re-run |
| Tag already exists | You need to bump `XELP_VERSION` -- you can't re-tag |
| Release workflow fails | Check Actions tab on GitHub; use `--release-local` as fallback |
| Network error mid-run | Safe to re-run -- the script checks state before each step |

## Fallback: local release

If GitHub Actions is down or the release workflow fails:

```bash
bash tools/make_release.sh --release-local
```

This does the full flow but creates the GitHub Release locally via `gh release
create` instead of waiting for the release workflow.

## Version format

`XELP_VERSION` is a 32-bit hex constant: `0x00MMmmpp` (major, minor, patch,
one byte each). The `extract_version.c` tool resolves this via the C
preprocessor -- no regex parsing. When patch is zero, the version string
omits it (e.g. `0x00010000` = `v1.0`, not `v1.0.0`).

See `release_management.md` for the full versioning and branching model.
