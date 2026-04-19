# Release Process

How to ship a release of xelp. The script `tools/make_release.sh` automates
every step below and pauses between each one so you can watch.

## Prerequisites

- `gcc` (build + tests)
- `gcov` (coverage)
- `gh` CLI, authenticated (`gh auth status`)
- `pio` CLI (optional, for PlatformIO publishing): `pip install platformio`
- PlatformIO account, logged in: `pio account login`
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
| 1b. Sync manifests | Updates `version` in `library.json` and `library.properties` to match `xelp.h` | No |
| 1c. Update badges | Updates version badges in README and pages | No |
| 2. Local validation | Clean build, runs all tests, checks zero warnings, checks coverage | No |
| 3. Git status | Shows uncommitted changes. If dirty, prompts you to commit or abort. | Yes, if dirty |
| 4. Push branch | Pushes current branch to origin | Yes |
| 5. Open PR | Creates a PR to `master` (or shows existing PR) | Yes |
| 6. Wait for CI | Polls `gh pr checks` until all checks pass or fail | Auto (Ctrl-C to abort) |
| 7. Auto-merge | Enables auto-merge (squash). Merge happens when CI passes. | Yes |
| 8. Wait + switch | Polls until PR merges, then checks out master and pulls | Auto |
| 9. Verify on master | Clean build + tests on the merged master | No |
| 10. Tag + push | Creates annotated tag `vX.Y.Z`, pushes to origin | Yes |
| 11. Wait for release | Polls until GitHub Release appears (created by `release.yml`) | Auto (Ctrl-C to abort) |
| 12. PlatformIO | Publishes to PlatformIO registry via `pio pkg publish` | Yes |
| 13. Done | Prints release URL and Arduino Library Manager reminder | No |

If you are already on `master` with a clean tree (e.g. you merged the PR
manually), the script detects this and skips steps 4-8.

## Before you start

1. Bump `XELP_VERSION` in `src/xelp.h` to the new version (single source of truth)
2. Update `CHANGELOG.md` -- move `[Unreleased]` items to a versioned heading
3. Commit those changes on your working branch

You do **not** need to manually edit `library.json` or `library.properties` --
the script syncs their `version` fields from `xelp.h` automatically.

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
| PlatformIO publish fails | Run `pio pkg publish .` manually after fixing the issue |
| `pio` not installed | Script skips PlatformIO and prints install instructions |
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

## PlatformIO registry

The release script publishes to PlatformIO automatically if `pio` is installed
and you're logged in. The package metadata lives in `library.json` at the repo
root.

To publish manually:

```bash
pio pkg publish .
```

Once a version is published it is **immutable** -- you cannot overwrite it.
Bump `XELP_VERSION` and release again if you need to fix something.

To check your published package:

```bash
pio pkg show deftio/xelp
```

## Arduino Library Manager

Arduino Library Manager indexes libraries from GitHub. One-time setup:

1. Fork https://github.com/arduino/library-registry
2. Add `libraries/xelp.json` containing:
   ```json
   { "repository": "https://github.com/deftio/xelp" }
   ```
3. Open a PR. Arduino's CI validates your `library.properties`.

Once merged, new versions are picked up automatically from GitHub
release tags. No manual action needed for subsequent releases.
