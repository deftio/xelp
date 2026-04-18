# Release Management

Reference for building, testing, and releasing xelp.

## Version Source of Truth

The `XELP_VERSION` macro in `src/xelp.h` is the single source of truth:

```c
#define XELP_VERSION      (0x00000203UL) /* 32-bit: 0x00MMmmpp */
#define XELP_VER_MAJOR(v) (((v) >> 16) & 0xFF)
#define XELP_VER_MINOR(v) (((v) >>  8) & 0xFF)
#define XELP_VER_PATCH(v) ( (v)        & 0xFF)
```

The 32-bit hex format encodes `0x00MMmmpp` (major.minor.patch), one byte
each. Example: `0x00010000` = version 1.0.0, `0x00000203` = version 0.2.3.
Accessor macros resolve to constants at compile time on all targets.

The build tool `tools/extract_version.c` reads the version via the C
preprocessor and writes `build/xelp_version.yaml`.  All scripts and CI
workflows use this tool instead of parsing `xelp.h` with regex.

```bash
make version   # compile + run, writes build/xelp_version.yaml
```

## Quick Reference

| Command | Purpose |
| --- | --- |
| `make tests` | Build + run unit tests + gcov |
| `make coverage` | Tests + coverage summary |
| `make version` | Extract version to build/xelp_version.yaml |
| `make example` | Build + run posix ncurses example |
| `make clean` | Remove all build artifacts |
| `bash tools/make_release.sh` | Validate build for release (dry run) |
| `bash tools/make_release.sh --tag` | Validate + tag + push (CI creates release) |
| `bash tools/make_release.sh --release` | Validate + tag + push + local release (fallback) |
| `bash tools/crossbuild.sh` | Docker cross-compilation size report |

## Development Workflow

```bash
git checkout -b dev-my-feature master
# ... make changes ...
make clean && make tests        # must pass with zero warnings
make coverage                   # check coverage didn't drop
```

## Release Workflow

1. Bump `XELP_VERSION` in `src/xelp.h`
2. Update `CHANGELOG.md` with release notes
3. Commit on your working branch

Then run the guided release script:

```bash
bash tools/make_release.sh
```

The script walks through every step: local validation, push, PR, CI,
merge, tag, and waits for the GitHub Release to appear. It pauses for
confirmation before anything that affects the remote.

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

- **Build matrix**: Ubuntu + macOS, GCC + Clang
- **32-bit build**: Ubuntu with `-m32`
- **Zero-warning check**: Fails CI if any compiler warnings detected
- **Coverage**: gcov report on Ubuntu/GCC

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
bash tools/make_release.sh --tag   # creates + pushes tag
# GitHub Actions takes over: validates, creates release
```

Or for full local control:

```bash
bash tools/make_release.sh --release   # creates tag + release via gh CLI
```

## Cross-Compilation

```bash
bash tools/crossbuild.sh        # Docker-based, all targets
bash tools/compactbuilds.sh     # host-native (requires toolchains)
```

See `tools/README_TOOLS.md` for details.
