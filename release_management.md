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
| `bash tools/make_release.sh --tag` | Validate + create git tag |
| `bash tools/make_release.sh --release` | Validate + tag + GitHub release |
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

Then use the release script:

```bash
# Dry run -- validates build, tests, coverage, git state
bash tools/make_release.sh

# Create a local git tag (vX.Y based on XELP_VERSION)
bash tools/make_release.sh --tag

# Full release: tag + push + GitHub release via gh CLI
bash tools/make_release.sh --release
```

The script:
- Reads `XELP_VERSION` from xelp.h and computes the semver string
- Runs `make clean && make tests` and fails on any error or warning
- Checks gcov coverage
- Verifies the working tree is clean (for tag/release mode)
- Checks the tag doesn't already exist
- Creates an annotated git tag
- Optionally pushes and creates a GitHub release via `gh`

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

`.github/workflows/ci.yml` runs on every push and PR to `master`:

- **Build matrix**: Ubuntu + macOS, GCC + Clang
- **32-bit build**: Ubuntu with `-m32`
- **Coverage**: gcov report on Ubuntu/GCC

## Cross-Compilation

```bash
bash tools/crossbuild.sh        # Docker-based, all targets
bash tools/compactbuilds.sh     # host-native (requires toolchains)
```

See `tools/README_TOOLS.md` for details.
