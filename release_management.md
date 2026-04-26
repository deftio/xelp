# Release Management

Reference for building, testing, and releasing xelp.

## Version Source of Truth

The `XELP_VERSION` macro in `src/xelp.h` is the single source of truth:

```c
#define XELP_VERSION      (0x00000301UL) /* 32-bit: 0x00MMmmpp */
#define XELP_VER_MAJOR(v) (((v) >> 16) & 0xFF)
#define XELP_VER_MINOR(v) (((v) >>  8) & 0xFF)
#define XELP_VER_PATCH(v) ( (v)        & 0xFF)
```

The 32-bit hex format encodes `0x00MMmmpp` (major.minor.patch), one byte
each. Example: `0x00010000` = version 1.0.0, `0x00000301` = version 0.3.1.
Accessor macros resolve to constants at compile time on all targets.

The build tool `tools/extract_version.c` reads the version via the C
preprocessor and writes `build/xelp_version.yaml`.  All scripts and CI
workflows use this tool instead of parsing `xelp.h` with regex.

```bash
make version   # compile + run, writes build/xelp_version.yaml
```

## Quick Reference

### Local development (fast, no Docker)

| Command | Purpose |
| --- | --- |
| `make validate` | Tests + build all examples -- the everyday pre-push check |
| `make tests` | Unit tests + gcov only |
| `make examples` | Build all examples (no interactive launch) |
| `make example` | Build + run posix ncurses example (interactive) |
| `make coverage` | Tests + coverage summary |
| `make funcsizes` | Per-function compiled sizes (x86-32, ARM32) |
| `make sizes` | Feature profile compiled sizes (ARM + host) |
| `make fuzz` | Fuzz testing with libFuzzer (requires clang) |
| `make version` | Extract version to build/xelp_version.yaml |
| `make clean` | Remove test build artifacts |
| `make clean-all` | Clean tests + all examples |

### Pre-release (validate + update size tables, requires Docker)

| Command | Purpose |
| --- | --- |
| `make prerelease` | Validate + Docker cross-compile + update README size tables |

Runs `make validate`, then `tools/crossbuild.sh` (Docker), then
`tools/update_sizes.sh` to patch the compiled-size tables in README.md
and pages/index.html. Does not tag, push, or publish.

### Full release

| Command | Purpose |
| --- | --- |
| `bash tools/make_release.sh` | Full guided release pipeline (includes Docker cross-build) |
| `bash tools/make_release.sh --validate` | Local validation only (no git, no push) |
| `bash tools/make_release.sh --release-local` | Full flow, creates GH release locally (fallback) |
| `bash tools/crossbuild.sh` | Docker cross-compile only (standalone, writes `build/sizes.csv`) |

The cross-build step is expensive (~minutes, requires Docker). It runs
automatically during `make_release.sh` and is skipped gracefully if Docker
is unavailable. Day-to-day development uses `make validate` which takes
seconds.

## Development Workflow

```bash
git checkout -b dev-my-feature master
# ... make changes ...
make validate                   # tests + examples, zero warnings
make coverage                   # check coverage didn't drop
```

## Release Workflow

1. Bump `XELP_VERSION` in `src/xelp.h`
2. Update `CHANGELOG.md` with release notes
3. Commit on your working branch
4. Run the guided release script:
   ```bash
   bash tools/make_release.sh
   ```

The script handles everything end-to-end: extract version, validate
(tests + examples + warnings + coverage), sync manifests, update
badges, Docker cross-compilation, size table update, push branch,
open PR, wait for CI, merge, tag, and wait for the GitHub Release.
It pauses for confirmation before anything that affects the remote.

If Docker is not installed, the cross-build step is skipped and
existing size tables are preserved.

For local validation only (no git, no PR):

```bash
bash tools/make_release.sh --validate
```

For manual fallback (e.g. GitHub Actions unavailable):

```bash
bash tools/make_release.sh --release-local
```

See `tools/make-release.md` for full documentation and troubleshooting.

## Branching Model

| Branch | Purpose | Merges to |
| --- | --- | --- |
| `master` | Stable, release-ready code | -- |
| `dev-*` / `feature-*` | Feature development | `master` via PR |
| `fix-*` | Bug fixes | `master` via PR |

Rules:
- `master` is always buildable with all tests passing
- All changes reach `master` through pull requests
- PRs require passing CI before merge
- Tag releases on `master` after merge

## CI / GitHub Actions

### Build CI (`.github/workflows/ci.yml`)

Runs on every push and PR to `master`:

- **Build matrix**: Ubuntu + macOS, GCC + Clang — each runs `make validate`
- **Coverage**: gcov report on Ubuntu/GCC
- CI mirrors local validation exactly. No extra jobs beyond `make validate`.

### Release CI (`.github/workflows/release.yml`)

Runs automatically when a version tag (`v*`) is pushed:

1. Verifies the tag matches `XELP_VERSION` in `src/xelp.h`
2. Builds and runs full test suite
3. Checks for zero compiler warnings
4. Runs coverage report
5. Extracts release notes from `CHANGELOG.md`
6. Creates a GitHub Release with the tag and notes

This means the release flow is:

```bash
bash tools/make_release.sh               # full guided release (includes tag + push)
# GitHub Actions takes over: validates, creates release
```

Or for full local control (if GitHub Actions is unavailable):

```bash
bash tools/make_release.sh --release-local   # creates tag + release via gh CLI
```

## Cross-Compilation

```bash
bash tools/crossbuild.sh                    # Docker-based, all targets -> build/sizes.csv
bash tools/crossbuild.sh --build            # force rebuild the Docker image
```

The cross-build is also run automatically as part of `make_release.sh`.
See `tools/README_TOOLS.md` for details.
