# Release Management

Reference for building, testing, and releasing xelp.

## Version Source of Truth

The `XELP_VERSION` macro in `src/xelp.h` is the single source of truth:

```c
#define XELP_VERSION (0x0021)  /* HEX: 0xMMmm -> major.minor */
```

The hex format encodes major version in the upper byte and minor in the
lower byte. Example: `0x0100` = version 1.0, `0x0021` = version 0.33.

## Quick Reference

| Command | Purpose |
| --- | --- |
| `make tests` | Build + run unit tests + gcov |
| `make coverage` | Tests + coverage summary |
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
3. Commit and push to a release branch
4. Open PR to master, get CI green, merge

Then release:

```bash
# 1. Dry run -- validates build, tests, coverage, git state
bash tools/make_release.sh

# 2. Tag + push -- CI creates the GitHub Release automatically
bash tools/make_release.sh --tag
```

The `--tag` path is the recommended workflow. The script validates locally
(clean build, zero warnings, all tests pass, coverage, clean git, no
duplicate tag), then creates an annotated tag and pushes it. GitHub Actions
picks up the tag push, validates again in CI, and creates the GitHub Release
with notes extracted from `CHANGELOG.md`.

For manual override (e.g. CI unavailable):

```bash
bash tools/make_release.sh --release   # tag + push + gh release locally
```

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
