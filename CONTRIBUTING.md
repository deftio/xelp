# Contributing to Xelp

Thanks for your interest in contributing. Xelp is a small, focused,
embedded-targeted CLI and script interpreter library. We want to keep it
small, portable, and reliable — so the bar for new features is "does
every embedded C programmer want this?" and the bar for bug fixes is
"are you making something that used to work wrong now work right?"

## Quick start

```bash
git clone https://github.com/deftio/xelp.git
cd xelp
make tests         # build and run the unit test suite
make clean         # remove build artifacts
```

All tests should pass on a clean checkout. If they don't, stop and
file an issue — we don't accept PRs on top of a broken baseline.

## What we welcome

- **Bug fixes** backed by a failing test case that now passes.
- **New test cases** for edge-case behavior that isn't yet covered.
- **Platform ports** (new compilers, new architectures) — especially
  if you can demonstrate the test suite passing on the new target.
- **Documentation fixes** in README.md, inline in headers, or in
  `docs/`.
- **Size optimizations** for specific embedded targets, gated behind
  `#ifdef` so the generic path stays readable.

## What we're cautious about

- **New public functions**: we try to keep the API surface small.
  Open an issue to discuss before writing the code.
- **Dependencies**: the library intentionally has zero dependencies.
  Anything that requires `<stdio.h>`, `<string.h>`, dynamic allocation,
  or a C++ runtime is a hard no for the core library.
- **Portability breaks**: the library is meant to compile on tiny C89
  toolchains (8051, MSP430, AVR, old ARM) as well as modern x64/ARM64
  hosts. Don't use GCC extensions, don't use VLAs, don't assume
  specific int sizes without checking.
- **Style rewrites**: we're not going to accept a PR that moves
  braces around or renames variables for aesthetic reasons.

## Ground rules for pull requests

### 1. Every behavior change needs a test

If you change what a function returns for any input, you must add or
update a test case in `tests/xelp_unit_tests.c` that covers the change.

### 2. No dynamic memory

The core library must never call `malloc`, `free`, `new`, `delete`, or
any other dynamic allocator. All state lives in the `XELP` instance
struct or is statically allocated by the user.

### 3. C89/C90 compatibility

The core `src/` files must compile under C89/C90. This means:
- No `//` comments in `.h` or `.c` files (use `/* */`)
- No mixed declarations and code
- No C99-only features in the core

### 4. Portable type usage

Use `int` for general values (xelp already does this intentionally
for portability across 8-bit to 64-bit targets). The `XELPREG` typedef
in `xelpcfg.h` allows users to override the register type for their
platform.

### 5. No globals

All state must go through the `XELP` instance pointer. Multiple
instances must be able to run independently with separate state.

### 6. Commit message format

```
component: short imperative summary (<= 72 chars)

Longer explanation if needed. Wrap at 72 chars. Explain what
changed and WHY, not just what.

Fixes #123  (if applicable)
```

Examples:
- `parser: fix buffer overflow on max-length command input`
- `tokenizer: handle escaped semicolons in quoted strings`
- `tests: add edge-case coverage for backspace at buffer start`

Avoid: `update stuff`, `fix`, `wip`, `clean up code`.

## Building and testing

```bash
make validate           # tests + build all examples -- the everyday check
make tests              # unit tests + gcov only
make examples           # build all examples (no interactive launch)
make coverage           # tests + coverage summary
make clean-all          # remove all build artifacts including examples
```

`make validate` is the recommended pre-push check. It runs the full test
suite with `-Werror` and builds all examples, confirming zero warnings
everywhere. Takes seconds on any modern machine.

The test suite uses the jumpbug framework. All tests must pass with
**zero compiler warnings** and **100% line coverage** of `xelp.c`
before a PR will be accepted.

The full `xelp.c.gcov` file is written to `build/` for line-by-line
details. Don't let coverage drop when adding new code -- add tests.

## Compile-time configuration

Xelp's feature set is controlled by `#define` flags in `src/xelpcfg.h`.
When adding new optional features:

1. Gate the feature behind a `#define XELP_ENABLE_*` flag.
2. Ensure the library still compiles and tests pass with the feature
   disabled.
3. Document the code size impact in the PR description.

## Branching and pull requests

`master` is the release-ready branch and is **protected**. Only
maintainers can merge to master. All changes go through pull requests.

```
master              (protected -- maintainer merge only)
  └── dev-feature   (your working branch)
```

Workflow:

1. Fork or clone the repo.
2. Create a feature branch from `master`:
   ```bash
   git checkout -b dev-my-feature master
   ```
3. Make your changes. Ensure `make tests` passes with zero warnings
   and coverage doesn't drop.
4. Push your branch and open a PR against `master`.
5. CI will run automatically (Ubuntu + macOS, GCC + Clang, 32-bit).
   All checks must pass.
6. A maintainer will review and merge.

## Release process

Releases are driven by `XELP_VERSION` in `src/xelp.h` -- this is the
single source of truth for the version number. The hex format encodes
`0x00MMmmpp` (major.minor.patch), e.g. `0x00000201` = version 0.2.1.

Only maintainers create releases. The process is:

1. Bump `XELP_VERSION` in `src/xelp.h`
2. Move the `[Unreleased]` section in `CHANGELOG.md` to a versioned
   heading (e.g. `## [0.3.1] - 2025-07-01`)
3. Commit on your working branch
4. (Optional) Run Docker cross-build to update size tables:
   ```bash
   bash tools/crossbuild.sh            # writes build/sizes.csv
   ```
5. Run the guided release script:
   ```bash
   bash tools/make_release.sh          # full guided release
   bash tools/make_release.sh --validate   # local validation only
   ```
6. GitHub Actions picks up the tag push, validates again in CI, and
   creates a GitHub Release with notes from `CHANGELOG.md`.

See `release_management.md` for full details.

## CI

Every push and PR to `master` triggers GitHub Actions
(`.github/workflows/ci.yml`):

- Build matrix: Ubuntu + macOS, GCC + Clang
- 32-bit build: Ubuntu with `-m32`
- Zero-warning enforcement
- Coverage report (Ubuntu/GCC)

Tag pushes (`v*`) trigger the release workflow
(`.github/workflows/release.yml`) which validates and creates a
GitHub Release automatically.

## License

By contributing you agree that your work will be released under the
same BSD 2-Clause license as the rest of the library. Don't include
code that you didn't write unless it is compatibly licensed and
attributed.

## Code of conduct

Be kind. Assume good faith. Keep discussion focused on the code.

## Questions?

Open an issue. We'd rather answer a question up front than merge a
PR that has to be reverted.
