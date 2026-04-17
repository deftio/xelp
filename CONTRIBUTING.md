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

## Running tests

```bash
make tests
```

This builds and runs the unit test suite using the jumpbug test
framework. All tests must pass.

### Coverage

```bash
cd tests
bash run_coverage_test.sh
```

This runs the tests with gcov instrumentation and produces
`xelp.c.gcov` showing line-by-line coverage. Don't let coverage drop
when adding new code — add tests.

## Compile-time configuration

Xelp's feature set is controlled by `#define` flags in `src/xelpcfg.h`.
When adding new optional features:

1. Gate the feature behind a `#define XELP_ENABLE_*` flag.
2. Ensure the library still compiles and tests pass with the feature
   disabled.
3. Document the code size impact in the PR description.

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
