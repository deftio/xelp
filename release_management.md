# Release Management

Reference for every helper script and `make` target used to build, test,
validate, and ship xelp. If a command is not listed here, it isn't part
of the supported release workflow.

The single source of truth for the current version is the repo-root
`VERSION` file (one line, e.g. `0.2.1`). The hex version in `src/xelp.h`
(`XELP_VERSION`) must match.

---

## Quick reference

| Command | Purpose |
| --- | --- |
| `make` | Build library + tests (default target) |
| `make tests` | Build + run the full unit test suite |
| `make example` | Build + run the posix example |
| `make clean` | Remove all build artifacts |

---

## Repository structure

```
xelp/
├── src/                    Core library (xelp.c, xelp.h, xelpcfg.h)
├── tests/                  Unit tests (jumpbug framework)
│   └── qa_visual_report/   HTML test report assets (retained for future use)
├── examples/               Platform-specific examples (posix, arduino)
├── tools/                  Code generators and build utilities
├── scripts/                Build and run helper scripts
├── docs/                   Documentation
├── dev/                    Design notes and development planning
├── img/                    Branding and icon assets
├── site/                   Project web page (index.html, libs/)
├── .github/workflows/      GitHub Actions CI
├── makefile                Build system
├── VERSION                 Single source of truth for version
├── LICENSE.txt             BSD 2-Clause license
└── README.md               Project overview
```

---

## Branching model

### Branch types

| Branch | Purpose | Merges to |
| --- | --- | --- |
| `master` | Stable, release-ready code | — |
| `dev-*` or `feature-*` | Feature development | `master` via PR |
| `fix-*` | Bug fixes | `master` via PR |
| `release-*` | Release preparation | `master` via PR |

### Rules

- `master` is always buildable and all tests pass.
- All changes reach `master` through pull requests.
- PRs require passing CI before merge.
- Squash-merge feature branches to keep history clean.
- Tag releases on `master` after merge.

---

## Development workflow

### Daily development

```bash
git checkout -b dev-my-feature master
# ... make changes ...
make tests                     # build + run tests
make clean && make tests       # clean rebuild + tests
```

### Before submitting a PR

1. Ensure all tests pass: `make tests`
2. Verify no compiler warnings: check `make tests` output for warnings
3. Run coverage if changing core code:
   ```bash
   cd tests && bash run_coverage_test.sh
   ```
4. If adding new features, add corresponding test cases in
   `tests/xelp_unit_tests.c`

---

## Release workflow

A typical version bump and release:

```bash
# 1. Create a release branch
git checkout -b release-X.Y.Z master

# 2. Update VERSION file
echo "X.Y.Z" > VERSION

# 3. Update XELP_VERSION in src/xelp.h to match
#    Format: 0xMMmm where MM=major, mm=minor (e.g. 0x0300 for 3.0.0)

# 4. Update CHANGELOG.md with release notes

# 5. Run full validation
make clean && make tests

# 6. Commit version bump
git add VERSION src/xelp.h CHANGELOG.md
git commit -m "release: bump version to X.Y.Z"

# 7. Open PR to master, get CI green, squash-merge

# 8. Tag the release on master
git checkout master
git pull
git tag -a vX.Y.Z -m "Release X.Y.Z"
git push origin vX.Y.Z
```

---

## Make targets

### Build

| Target | Effect |
| --- | --- |
| `make tests` | Compile library + test framework + unit tests, run tests, generate coverage |
| `make example` | Compile library + posix example, run example |
| `make clean` | Remove all `.o`, `.gcda`, `.gcno`, `.gcov`, `.out` files |

### Test files

| File | Purpose |
| --- | --- |
| `tests/xelp_unit_tests.c` | Main unit test suite |
| `tests/jumpbug_unit_test_fw.c` | Lightweight C unit test framework |
| `tests/jumpbug_unit_test_fw.h` | Test framework header |
| `tests/run_coverage_test.sh` | Script to generate gcov coverage report |

---

## CI / GitHub Actions

The `.github/workflows/ci.yml` workflow runs on every push and PR to
`master`:

- **Build matrix**: Ubuntu + macOS, GCC + Clang
- **32-bit build**: Ubuntu with `-m32` to verify embedded-size int behavior
- **Coverage**: gcov report on Ubuntu/GCC

CI must be green before merging any PR to `master`.

---

## Versioning

Xelp uses semantic versioning: `MAJOR.MINOR.PATCH`

- **MAJOR**: Breaking API changes (struct layout, function signatures)
- **MINOR**: New features, backward-compatible
- **PATCH**: Bug fixes, documentation, build improvements

The version appears in two places that must stay in sync:

1. `VERSION` file (plain text, e.g. `0.2.1`)
2. `src/xelp.h` `XELP_VERSION` macro (hex, e.g. `0x0021`)

---

## Coverage

Current coverage tooling uses `gcov` (part of the GCC suite). To generate
a coverage report:

```bash
make clean
make tests
gcov src/xelp.c
```

The resulting `xelp.c.gcov` file shows line-by-line execution counts.
Lines marked `#####` were never executed and represent coverage gaps.

Future plans include lcov HTML reports and coverage badge generation.

---

## Cross-compilation

Xelp targets embedded platforms. While CI runs on desktop x86/ARM,
the library is designed to compile on:

| Architecture | Compiler | Notes |
| --- | --- | --- |
| x86-32/64 | GCC, Clang | Primary development/test platform |
| ARM32/Thumb | arm-none-eabi-gcc | MBED, Raspberry Pi |
| MSP430 | msp430-gcc | 16-bit, minimal footprint |
| AVR | avr-gcc | Arduino |
| 8051 | SDCC | Uses `__reentrant` qualifier |
| 6502 | cc65 | 8-bit targets |
| PIC18F | SDCC | Microchip |
| 68HC11/12 | m68hc1x-gcc | Motorola/Freescale |

Cross-compilation is done with `tools/compactbuilds.sh` or manually:

```bash
arm-none-eabi-gcc -c src/xelp.c -Os -mthumb -Isrc
msp430-gcc -c src/xelp.c -Os -Isrc
```

---

## See also

- `CONTRIBUTING.md` — PR expectations, portability rules, commit format
- `CHANGELOG.md` — per-release change log
- `VERSION` — single source of truth for the version string
- `dev/xelp-plan-2025.md` — comprehensive modernization roadmap
